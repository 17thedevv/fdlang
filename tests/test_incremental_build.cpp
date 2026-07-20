// =============================================================================
// tests/test_incremental_build.cpp
//
// Tests for Incremental Build system:
//   1. FNV-1a hashing is deterministic and sensitive to input changes.
//   2. computeBuildPlan() marks a module as needsRebuild when no .mlib exists.
//   3. After markModuleBuilt(), the same module is NOT rebuilt (cache hit).
//   4. "Snowball Effect": changing a dependency causes its consumer to rebuild.
// =============================================================================

#include "mellis/Core/HashUtils.h"
#include "mellis/Core/WorkspaceBuilder.h"
#include "mellis/Support/Diagnostic.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ── Helper: create a minimal valid MLIB file (header only) ─────────────────
// Layout (packed LE):
//   [0 ] char magic[4]       = "MLIB"
//   [4 ] u16 formatVersion   = 1
//   [6 ] u16 compilerVersion = 1
//   [8 ] char targetTriple[64] = ""
//   [72] u8 uuid[16]         = 0
//   [88] u64 moduleHash      = 0   ← to be patched by markModuleBuilt
//   [96] u64 timestamp       = 0
//   [104] u32 flags          = 0
//   [108] u32 sectionCount   = 0
//   [112] u64 sectionTableOffset = 0
static void createFakeMLib(const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    // magic
    f.write("MLIB", 4);
    // formatVersion + compilerVersion
    uint8_t u16be[2] = {1, 0};
    f.write(reinterpret_cast<const char*>(u16be), 2); // formatVersion = 1
    f.write(reinterpret_cast<const char*>(u16be), 2); // compilerVersion = 1
    // targetTriple[64]
    char zeros[64] = {};
    f.write(zeros, 64);
    // uuid[16]
    f.write(zeros, 16);
    // moduleHash (8 bytes) = 0
    f.write(zeros, 8);
    // timestamp (8) + flags (4) + sectionCount (4) + sectionTableOffset (8) = 24 bytes
    f.write(zeros, 24);
}

// ── Test 1: FNV-1a is deterministic and change-sensitive ───────────────────
static void testHashUtils() {
    using namespace fl;

    uint64_t h1 = fnv1a64("hello world");
    uint64_t h2 = fnv1a64("hello world");
    assert(h1 == h2 && "Hash must be deterministic");

    uint64_t h3 = fnv1a64("hello World"); // Capital W
    assert(h1 != h3 && "Hash must differ for different inputs");

    // Empty string should produce a known value (FNV offset basis)
    uint64_t h4 = fnv1a64("");
    assert(h4 == FNV1A_OFFSET_BASIS && "Empty string should yield offset basis");

    // calculateModuleHash with no deps = fnv1a64(src)
    uint64_t h5 = calculateModuleHash("fn main() {}", {});
    assert(h5 == fnv1a64("fn main() {}") && "No-dep hash must equal content hash");

    // combineHashes: adding a dep changes the result
    uint64_t h6 = calculateModuleHash("fn main() {}", {12345ULL});
    assert(h5 != h6 && "Adding a dep must change the combined hash");

    std::cout << "testHashUtils passed\n";
}

// ── Test 2: computeBuildPlan marks modules as dirty when no .mlib ──────────
static void testNoCacheForcesBuild() {
    fs::path tmp = fs::temp_directory_path() / "mellis_test_incr_build";
    fs::create_directories(tmp);

    // Create a fake workspace with a single .ms file
    fs::path srcDir = tmp / "src";
    fs::create_directories(srcDir);
    fs::path outputDir = tmp / "cache";
    fs::create_directories(outputDir);

    (std::ofstream(srcDir / "math.ms")) << "fn add(a: int_32, b: int_32) -> int_32 { return a + b; }";

    fl::DiagnosticEngine diag;
    fl::WorkspaceBuilder wb(diag, srcDir.string());
    bool ok = wb.buildGraphFromEntry("math");
    assert(ok && "buildGraphFromEntry should succeed");

    auto plan = wb.computeBuildPlan(outputDir.string());
    assert(!plan.empty() && "Plan should not be empty");

    auto& mathStatus = plan[0];
    assert(mathStatus.moduleName == "math");
    assert(mathStatus.cachedHash == 0 && "No .mlib exists → cachedHash must be 0");
    assert(mathStatus.needsRebuild && "No .mlib → must rebuild");

    std::cout << "testNoCacheForcesBuild passed\n";
}

// ── Test 3: After markModuleBuilt(), same module is NOT rebuilt ────────────
static void testCacheHitSkipsRebuild() {
    fs::path tmp = fs::temp_directory_path() / "mellis_test_cache_hit";
    fs::create_directories(tmp);

    fs::path srcDir = tmp / "src";
    fs::create_directories(srcDir);
    fs::path outputDir = tmp / "cache";
    fs::create_directories(outputDir);

    std::string sourceContent = "fn greet() {}";
    (std::ofstream(srcDir / "greeter.ms")) << sourceContent;

    fl::DiagnosticEngine diag;
    fl::WorkspaceBuilder wb(diag, srcDir.string());
    wb.buildGraphFromEntry("greeter");

    // First plan → needsRebuild = true
    auto plan1 = wb.computeBuildPlan(outputDir.string());
    assert(plan1[0].needsRebuild && "First build: must rebuild");

    uint64_t hash = plan1[0].currentHash;
    std::string mlibPath = plan1[0].outputFile;

    // Simulate: create fake .mlib then patch hash into it
    createFakeMLib(mlibPath);
    bool patched = fl::WorkspaceBuilder::markModuleBuilt(mlibPath, hash);
    assert(patched && "markModuleBuilt should succeed on a valid MLIB file");

    // Verify what was written
    uint64_t readBack = fl::WorkspaceBuilder::readCachedHash(mlibPath);
    assert(readBack == hash && "readCachedHash must return the patched hash");

    // Second plan with same source → needsRebuild = false
    fl::DiagnosticEngine diag2;
    fl::WorkspaceBuilder wb2(diag2, srcDir.string());
    wb2.buildGraphFromEntry("greeter");
    auto plan2 = wb2.computeBuildPlan(outputDir.string());
    assert(!plan2[0].needsRebuild && "Cache hit: must NOT rebuild");

    std::cout << "testCacheHitSkipsRebuild passed\n";
}

// ── Test 4: Snowball effect — changing dep forces consumer to rebuild ───────
static void testSnowballEffect() {
    fs::path tmp = fs::temp_directory_path() / "mellis_test_snowball";
    fs::create_directories(tmp);

    fs::path srcDir = tmp / "src";
    fs::create_directories(srcDir);
    fs::path outputDir = tmp / "cache";
    fs::create_directories(outputDir);

    // math.ms ← main.ms depends on math
    std::string mathSrc  = "fn add(a: int_32, b: int_32) -> int_32 { return a + b; }";
    std::string mainSrc  = "use math;\nfn main() {}";
    (std::ofstream(srcDir / "math.ms")) << mathSrc;
    (std::ofstream(srcDir / "main.ms")) << mainSrc;

    fl::DiagnosticEngine diag;
    fl::WorkspaceBuilder wb(diag, srcDir.string());
    wb.buildGraphFromEntry("main");

    // Build plan: both math and main are dirty (no .mlib yet)
    auto plan1 = wb.computeBuildPlan(outputDir.string());
    assert(plan1.size() == 2);

    // Simulate building both: create fake .mlib + patch hash for math then main
    for (auto& s : plan1) {
        createFakeMLib(s.outputFile);
        fl::WorkspaceBuilder::markModuleBuilt(s.outputFile, s.currentHash);
    }

    // Now nothing changed — both should be up-to-date
    fl::DiagnosticEngine diag2;
    fl::WorkspaceBuilder wb2(diag2, srcDir.string());
    wb2.buildGraphFromEntry("main");
    auto plan2 = wb2.computeBuildPlan(outputDir.string());
    for (auto& s : plan2) {
        assert(!s.needsRebuild && "No changes → nothing should rebuild");
    }

    // === SNOWBALL: Modify math.ms ===
    (std::ofstream(srcDir / "math.ms")) << mathSrc + " // changed";

    fl::DiagnosticEngine diag3;
    fl::WorkspaceBuilder wb3(diag3, srcDir.string());
    wb3.buildGraphFromEntry("main");
    auto plan3 = wb3.computeBuildPlan(outputDir.string());

    // Find math and main statuses
    fl::ModuleBuildStatus* mathStat = nullptr;
    fl::ModuleBuildStatus* mainStat = nullptr;
    for (auto& s : plan3) {
        if (s.moduleName == "math") mathStat = &s;
        if (s.moduleName == "main") mainStat = &s;
    }
    assert(mathStat && "math must be in plan");
    assert(mainStat && "main must be in plan");
    assert(mathStat->needsRebuild && "math source changed → must rebuild math");
    assert(mainStat->needsRebuild && "math changed → snowball → main must also rebuild");

    std::cout << "testSnowballEffect passed\n";
}

// ── main ───────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== Incremental Build Tests ===\n";
    testHashUtils();
    testNoCacheForcesBuild();
    testCacheHitSkipsRebuild();
    testSnowballEffect();
    std::cout << "All Incremental Build tests passed!\n";
    return 0;
}
