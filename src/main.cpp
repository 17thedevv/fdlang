#include <iostream>
#include <string>
#include "mellis/Core/CompilerSession.h"

using namespace fl;

int main(int argc, char* argv[]) {
    bool verbose = false;
    int optLevel = 0;
    std::string filepath = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--version") {
            std::cout << "mellis version 0.1.0\n";
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-O1") {
            optLevel = 1;
        } else if (filepath.empty() && arg[0] != '-') {
            filepath = arg;
        }
    }

    if (filepath.empty()) {
        std::cerr << "mellis: error: Cach su dung: mellis <file_path.ms> [-v|--verbose] [--version]\n";
        return 1;
    }

    std::cout << "[MELLIS COMPILER]" << std::endl;

    CompilerSession session;
    bool success = session.compile(filepath, verbose, optLevel);

    if (!success) {
        return 65; // data format error
    }

    return 0;
}
