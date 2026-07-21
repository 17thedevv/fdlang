// =============================================================================
// mellis/MLib/MLibGenerator.cpp
// =============================================================================

#include "mellis/MLib/MLibGenerator.h"
#include "mellis/MLib/MLibFormat.h"
#include "mellis/MLib/BinaryWriter.h"
#include "mellis/MLib/StringTableBuilder.h"
#include "mellis/MLib/ManifestBuilder.h"
#include "mellis/MLib/MetadataBuilder.h"
#include "mellis/MLib/MacroMetadataBuilder.h"
#include "mellis/MLib/ObjectCodeBuilder.h"

#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Passes/PassBuilder.h>

#include <fstream>
#include <iostream>

namespace fl {

MLibGenerator::MLibGenerator(DiagnosticEngine& diag, SymbolTable& symTab, TypeContext& typeCtx, MacroRegistry& macroReg)
    : diag_(diag), symTab_(symTab), typeCtx_(typeCtx), macroReg_(macroReg) {}

bool MLibGenerator::generate(llvm::Module* llvmModule, const std::string& outputPath) {
    using namespace fl::mlib;

    // 1. Generate Object Code (.obj) in memory
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    llvmModule->setTargetTriple(targetTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        diag_.error(SourceLocation{}, "MLibGenerator: Không thể tìm thấy Target: " + error);
        return false;
    }

    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions opt;
    auto rm = std::optional<llvm::Reloc::Model>();
    auto targetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, rm);

    llvmModule->setDataLayout(targetMachine->createDataLayout());

    // Run Optimization
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
    llvm::PassBuilder PB(targetMachine);
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O1);
    MPM.run(*llvmModule, MAM);

    // Emit object code to buffer
    llvm::SmallVector<char, 0> objBuffer;
    llvm::raw_svector_ostream objStream(objBuffer);
    llvm::legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, objStream, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        diag_.error(SourceLocation{}, "TargetMachine không hỗ trợ phát sinh object file.");
        return false;
    }
    pass.run(*llvmModule);

    // 2. Build MLib Sections
    
    // a. String Table
    StringTableBuilder strings;
    
    // Get module name from filename
    std::string moduleName = outputPath;
    size_t lastSlash = moduleName.find_last_of("/\\");
    if (lastSlash != std::string::npos) moduleName = moduleName.substr(lastSlash + 1);
    size_t lastDot = moduleName.find_last_of('.');
    if (lastDot != std::string::npos) moduleName = moduleName.substr(0, lastDot);
    
    // Manifest
    ManifestBuilder manifest(strings);
    manifest.setPackageName(moduleName);
    manifest.setVersion("0.1.0");
    BinaryWriter manifestWriter;
    manifest.serialize(manifestWriter);
    auto manifestBytes = manifestWriter.takeBuffer();

    // ExportTable (Empty for now)
    BinaryWriter exportWriter;
    exportWriter.writeU32(1); // version
    exportWriter.writeU32(0); // count
    auto exportBytes = exportWriter.takeBuffer();

    // TypeMetadata (Empty for now)
    BinaryWriter typeWriter;
    typeWriter.writeU32(1); // version
    typeWriter.writeU32(0); // count
    auto typeBytes = typeWriter.takeBuffer();

    // TraitMetadata (Empty for now)
    BinaryWriter traitWriter;
    traitWriter.writeU32(1); // version
    traitWriter.writeU32(0); // count
    auto traitBytes = traitWriter.takeBuffer();
    
    // StringTable serialization
    BinaryWriter strWriter;
    strings.serialize(strWriter);
    auto strBytes = strWriter.takeBuffer();

    // Macro metadata (Empty for now)
    MacroMetadataBuilder macroBuilder;
    BinaryWriter macroWriter;
    macroBuilder.serialize(macroWriter);
    auto macroBytes = macroWriter.takeBuffer();

    // Object Code
    ObjectCodeBuilder objBuilder;
    objBuilder.addFunction(0, reinterpret_cast<const uint8_t*>(objBuffer.data()), objBuffer.size());
    BinaryWriter objWriter;
    objBuilder.serialize(objWriter);
    auto finalObjBytes = objWriter.takeBuffer();

    // ── Assemble the .mlib binary ─────────────────────────────────────────────
    const uint32_t NUM_SECTIONS = 7;
    const uint64_t headerSize   = sizeof(MLibHeader);
    const uint64_t tableSize    = NUM_SECTIONS * sizeof(SectionEntry);
    
    uint64_t offset = headerSize + tableSize;
    
    auto getNextOffset = [&offset](size_t size) {
        uint64_t start = offset;
        offset += size;
        return start;
    };

    uint64_t manifestOffset = getNextOffset(manifestBytes.size());
    uint64_t strOffset      = getNextOffset(strBytes.size());
    uint64_t exportOffset   = getNextOffset(exportBytes.size());
    uint64_t typeOffset     = getNextOffset(typeBytes.size());
    uint64_t traitOffset    = getNextOffset(traitBytes.size());
    uint64_t macroOffset    = getNextOffset(macroBytes.size());
    uint64_t objOffset      = getNextOffset(finalObjBytes.size());

    MLibHeader hdr{};
    std::memcpy(hdr.magic, MLIB_MAGIC, 4);
    hdr.formatVersion   = 1;
    hdr.compilerVersion = 1;
    // UUID (Mock deterministic UUID for now)
    for (int i = 0; i < 16; ++i) hdr.moduleUUID[i] = static_cast<uint8_t>(i + 1);
    hdr.sectionCount       = NUM_SECTIONS;
    hdr.sectionTableOffset = headerSize;

    auto makeSection = [](uint32_t id, SectionType type, uint64_t off, uint64_t sz) {
        SectionEntry e{};
        e.sectionID   = id;
        e.sectionType = static_cast<uint32_t>(type);
        e.offset      = off;
        e.size        = sz;
        e.version     = 1;
        return e;
    };

    SectionEntry sections[NUM_SECTIONS] = {
        makeSection(1, SectionType::Manifest,      manifestOffset, manifestBytes.size()),
        makeSection(2, SectionType::StringTable,   strOffset,      strBytes.size()),
        makeSection(3, SectionType::ExportTable,   exportOffset,   exportBytes.size()),
        makeSection(4, SectionType::TypeMetadata,  typeOffset,     typeBytes.size()),
        makeSection(5, SectionType::TraitMetadata, traitOffset,    traitBytes.size()),
        makeSection(6, SectionType::MacroMetadata, macroOffset,    macroBytes.size()),
        makeSection(7, SectionType::ObjectCode,    objOffset,      finalObjBytes.size()),
    };

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        diag_.error(SourceLocation{}, "Khong the ghi file .mlib: " + outputPath);
        return false;
    }

    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.write(reinterpret_cast<const char*>(sections), sizeof(sections));
    out.write(reinterpret_cast<const char*>(manifestBytes.data()), manifestBytes.size());
    out.write(reinterpret_cast<const char*>(strBytes.data()), strBytes.size());
    out.write(reinterpret_cast<const char*>(exportBytes.data()), exportBytes.size());
    out.write(reinterpret_cast<const char*>(typeBytes.data()), typeBytes.size());
    out.write(reinterpret_cast<const char*>(traitBytes.data()), traitBytes.size());
    out.write(reinterpret_cast<const char*>(macroBytes.data()), macroBytes.size());
    out.write(reinterpret_cast<const char*>(finalObjBytes.data()), finalObjBytes.size());

    return true;
}

} // namespace fl
