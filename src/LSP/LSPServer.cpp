// =============================================================================
// mellis/LSP/LSPServer.cpp
// =============================================================================

#include "mellis/LSP/LSPServer.h"
#include "mellis/Core/CompilerSession.h"
#include "mellis/Support/Diagnostic.h"
#include "mellis/Core/SourceManager.h"
#include "mellis/FrontEnd/Lexer.h"
#include "mellis/FrontEnd/Parser.h"
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>
#include <string>
#include <vector>

namespace fl {
namespace lsp {

// Ghi JSON-RPC message ra stdout theo chuẩn LSP
void sendMessage(const llvm::json::Object& msg) {
    std::string jsonStr;
    llvm::raw_string_ostream os(jsonStr);
    os << llvm::json::Value(llvm::json::Object(msg)); // Workaround for MSVC Value conversion
    os.flush();

    std::cout << "Content-Length: " << jsonStr.size() << "\r\n\r\n" << jsonStr;
    std::cout.flush();
}

// Chạy luồng phân tích để sinh Diagnostics
void publishDiagnostics(const std::string& uri, const std::string& sourceCode) {
    // Để giữ cho LSP phản hồi nhanh, chúng ta chỉ chạy qua TypeChecker
    // Chúng ta không gọi ExecutableGenerator hay LLVM IR Generator.

    // 1. Tạo SourceManager ảo
    DiagnosticEngine diag;
    SourceManager sourceManager(diag);
    diag.setSourceManager(&sourceManager);
    
    // Để tránh lỗi loadFile ko có addVirtualFile, ta lưu ra file tạm
    std::string tempPath = "lsp_temp.ms";
    if (FILE* f = fopen(tempPath.c_str(), "wb")) {
        fwrite(sourceCode.data(), 1, sourceCode.size(), f);
        fclose(f);
    }
    FileID fileId = sourceManager.loadFile(tempPath);

    // 2. Chạy Lexer & Parser
    Lexer lexer(sourceCode);
    Parser parser(lexer, diag, &sourceManager, fileId);
    auto ast = parser.parse();

    // 4. Lấy danh sách lỗi từ DiagnosticEngine
    llvm::json::Array diagnostics;

    for (const auto& err : diag.allDiagnostics()) {
        llvm::json::Object diagObj;
        
        int line = 0, col = 0;
        uint32_t offset = err.location.offset;
        if (offset != std::numeric_limits<uint32_t>::max()) {
            for (uint32_t i = 0; i < offset && i < sourceCode.size(); ++i) {
                if (sourceCode[i] == '\n') {
                    line++;
                    col = 0;
                } else {
                    col++;
                }
            }
        }

        llvm::json::Object startPos;
        startPos["line"] = line;
        startPos["character"] = col;

        llvm::json::Object endPos;
        endPos["line"] = line;
        endPos["character"] = col + 1;
        
        llvm::json::Object range;
        range["start"] = std::move(startPos);
        range["end"] = std::move(endPos);

        diagObj["range"] = std::move(range);
        diagObj["severity"] = 1; // 1 = Error
        diagObj["message"] = err.message;
        
        diagnostics.push_back(std::move(diagObj));
    }

    // 5. Gửi thông điệp textDocument/publishDiagnostics
    llvm::json::Object params;
    params["uri"] = uri;
    params["diagnostics"] = std::move(diagnostics);

    llvm::json::Object notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "textDocument/publishDiagnostics";
    notification["params"] = std::move(params);

    sendMessage(notification);
}

int runServer() {
    // VSCode giao tiếp qua stdin/stdout
    while (true) {
        int contentLength = 0;
        std::string line;
        
        // Đọc Header
        while (std::getline(std::cin, line)) {
            // Xóa \r nếu có
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) break; // Hết header

            if (line.rfind("Content-Length: ", 0) == 0) {
                contentLength = std::stoi(line.substr(16));
            }
        }

        if (std::cin.eof()) break; // Đóng kết nối
        if (contentLength == 0) continue;

        // Đọc Body
        std::string body;
        body.resize(contentLength);
        std::cin.read(&body[0], contentLength);

        // Phân tích JSON
        auto expectedJson = llvm::json::parse(body);
        if (!expectedJson) {
            llvm::consumeError(expectedJson.takeError());
            continue;
        }

        llvm::json::Object* request = expectedJson->getAsObject();
        if (!request) continue;

        auto methodOpt = request->getString("method");
        if (!methodOpt) continue;
        std::string method = methodOpt->str();

        if (method == "initialize") {
            auto id = request->get("id");
            // Phản hồi initialize báo rằng chúng ta hỗ trợ đồng bộ file (Full Sync)
            llvm::json::Object capabilities;
            capabilities["textDocumentSync"] = 1;
            llvm::json::Object result;
            result["capabilities"] = std::move(capabilities);
            llvm::json::Object response;
            response["jsonrpc"] = "2.0";
            response["id"] = *id;
            response["result"] = std::move(result);
            sendMessage(response);
        }
        else if (method == "textDocument/didOpen") {
            auto params = request->getObject("params");
            auto textDoc = params ? params->getObject("textDocument") : nullptr;
            if (textDoc) {
                auto uri = textDoc->getString("uri");
                auto text = textDoc->getString("text");
                if (uri && text) publishDiagnostics(uri->str(), text->str());
            }
        }
        else if (method == "textDocument/didChange") {
            auto params = request->getObject("params");
            auto textDoc = params ? params->getObject("textDocument") : nullptr;
            auto contentChanges = params ? params->getArray("contentChanges") : nullptr;
            if (textDoc && contentChanges && !contentChanges->empty()) {
                auto uri = textDoc->getString("uri");
                auto change = (*contentChanges)[0].getAsObject();
                if (uri && change) {
                    auto text = change->getString("text");
                    if (text) publishDiagnostics(uri->str(), text->str());
                }
            }
        }
        else if (method == "shutdown") {
            auto id = request->get("id");
            llvm::json::Object response;
            response["jsonrpc"] = "2.0";
            response["id"] = *id;
            response["result"] = nullptr;
            sendMessage(response);
        }
        else if (method == "exit") {
            break;
        }
    }

    return 0;
}

} // namespace lsp
} // namespace fl
