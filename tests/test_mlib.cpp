#include "mellis/MLib/MLibFormat.h"
#include "mellis/MLib/BinaryWriter.h"
#include "mellis/MLib/BinaryReader.h"
#include "mellis/MLib/Serializer.h"
#include "mellis/MLib/Deserializer.h"
#include "mellis/MLib/StringTableBuilder.h"
#include "mellis/MLib/ManifestBuilder.h"
#include "mellis/MLib/MetadataBuilder.h"
#include "mellis/MLib/GenericSerializer.h"
#include "mellis/MLib/GenericDeserializer.h"
#include "mellis/MLib/ObjectCodeBuilder.h"
#include "mellis/MLib/ObjectCodeExtractor.h"
#include "mellis/MLib/MacroMetadataBuilder.h"
#include "mellis/MLib/DebugInfoBuilder.h"
#include "mellis/MLib/DepGraphBuilder.h"
#include "mellis/MLib/DocSectionBuilder.h"
#include "mellis/MiddleEnd/MVIR.h"

#include <iostream>
#include <cassert>
#include <cstring>
#include <unordered_map>

using namespace fl::mlib;

void testBasicWriterReader() {
    BinaryWriter writer;
    writer.writeU8(0xAA);
    writer.writeU16(0xBBCC);
    writer.writeU32(0xDDEEFF00);
    writer.writeU64(0x1122334455667788ULL);
    writer.writeFloat(3.14159f);
    writer.writeDouble(2.718281828);

    auto buf = writer.takeBuffer();
    BinaryReader reader(buf.data(), buf.size());

    assert(reader.readU8() == 0xAA);
    assert(reader.readU16() == 0xBBCC);
    assert(reader.readU32() == 0xDDEEFF00);
    assert(reader.readU64() == 0x1122334455667788ULL);
    
    float f = reader.readFloat();
    assert(std::abs(f - 3.14159f) < 0.0001f);
    
    double d = reader.readDouble();
    assert(std::abs(d - 2.718281828) < 0.0000001);
}

void testStringTableDeduplication() {
    StringTableBuilder stringTable;
    
    uint32_t id1 = stringTable.addString("Hello");
    uint32_t id2 = stringTable.addString("World");
    uint32_t id3 = stringTable.addString("Hello"); // Should be deduplicated

    assert(id1 != id2);
    assert(id1 == id3);

    // Empty string is always 0
    uint32_t id4 = stringTable.addString("");
    assert(id4 == 0);

    BinaryWriter writer;
    stringTable.serialize(writer);

    auto buf = writer.takeBuffer();
    BinaryReader reader(buf.data(), buf.size());

    // We can manually read strings using the offset
    reader.seek(id1);
    std::string str1 = "";
    while (char c = static_cast<char>(reader.readU8())) {
        str1 += c;
    }
    assert(str1 == "Hello");
}

void testManifestAndDependency() {
    StringTableBuilder stringTable;
    ManifestBuilder manifest(stringTable);

    manifest.setPackageName("core_collections");
    manifest.setAuthor("Mellis Team");
    manifest.setVersion("1.0.0");
    manifest.setLicense("MIT");

    manifest.addFeature("simd");
    manifest.addFeature("async");

    uint8_t uuid[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    manifest.addDependency(uuid, "0.5.0", 0x12345678ULL, ImportMode::Public, {"allocator"});

    BinaryWriter writer;
    manifest.serialize(writer);

    auto buf = writer.takeBuffer();
    BinaryReader reader(buf.data(), buf.size());

    ManifestHeader header;
    reader.readStruct(header);

    assert(header.featureCount == 2);
    assert(header.dependencyCount == 1);

    uint32_t feat1 = reader.readU32();
    uint32_t feat2 = reader.readU32();
    // feat1 and feat2 map to "simd" and "async"

    DependencyEntry dep;
    reader.readStruct(dep);
    assert(dep.moduleUUID[0] == 1);
    assert(dep.moduleHash == 0x12345678ULL);
    assert(dep.importMode == ImportMode::Public);
    assert(dep.featureCount == 1);
}

void testMetadataArena() {
    StringTableBuilder stringTable;
    MetadataBuilder meta(stringTable);

    uint32_t nsID = meta.addNamespace("std");
    uint32_t typeID = meta.addType("Vec", nsID, 24, 8);
    uint32_t traitID = meta.addTrait("Display", nsID);
    uint32_t funcID = meta.addFunction("push", nsID, 0); // Dummy signature
    uint32_t implID = meta.addImpl(traitID, typeID);

    BinaryWriter writer;
    meta.serializeMetadata(writer);

    auto buf = writer.takeBuffer();
    BinaryReader reader(buf.data(), buf.size());

    uint32_t nsCount = reader.readU32();
    assert(nsCount == 1);
    NamespaceEntry nsEntry;
    reader.readStruct(nsEntry);
    assert(nsEntry.parentNamespaceID == 0xFFFFFFFF);

    uint32_t typeCount = reader.readU32();
    assert(typeCount == 1);
    TypeEntry typeEntry;
    reader.readStruct(typeEntry);
    assert(typeEntry.size == 24);
    assert(typeEntry.alignment == 8);

    uint32_t traitCount = reader.readU32();
    assert(traitCount == 1);
    TraitEntry traitEntry;
    reader.readStruct(traitEntry);

    uint32_t funcCount = reader.readU32();
    assert(funcCount == 1);
    FunctionEntry funcEntry;
    reader.readStruct(funcEntry);
    assert(funcEntry.namespaceID == nsID);
}

void testGenericSectionIndex() {
    using namespace fl::mvir;
    
    // Create a mock Generic Function (max<T>)
    Function func;
    func.name = GlobalId{"max"};
    
    auto bb = std::make_unique<BasicBlock>(LabelId{"bb0"});
    
    bb->instructions.push_back(std::make_unique<AluInst>(
        LocalId{"%res"}, AluOp::Add, LocalId{"%a"}, LocalId{"%b"}
    ));
    
    bb->terminator = std::make_unique<RetTerm>(LocalId{"%res"});
    
    func.blocks.push_back(std::move(bb));
    
    GenericSerializer serializer;
    std::vector<uint32_t> genericIDs = {100};
    std::vector<uint32_t> constraintIDs = {200};
    
    // Serialize package
    auto pkgBytes = serializer.serializePackage(17, genericIDs, constraintIDs, 50, func);
    
    // Manually construct the whole section
    BinaryWriter sectionWriter;
    sectionWriter.writeU32(1); // Version 1
    sectionWriter.writeU32(1); // Function Count
    
    // Index Table
    GenericFunctionIndex idx;
    idx.functionID = 17;
    idx.compressedSize = pkgBytes.size();
    idx.uncompressedSize = pkgBytes.size();
    // Calculate offset: Version(4) + Count(4) + IndexEntry(48)
    idx.offset = 4 + 4 + sizeof(GenericFunctionIndex);
    idx.mvirHash = 0xDEADBEEF12345678ULL;
    idx.constraintHash = 0;
    idx.signatureHash = 0;
    
    sectionWriter.writeStruct(idx);
    
    // Payload
    sectionWriter.writeBytes(pkgBytes.data(), pkgBytes.size());
    
    auto sectionBytes = sectionWriter.takeBuffer();
    
    // Test Deserialization & Lazy Loading
    GenericDeserializer deserializer(sectionBytes.data(), sectionBytes.size());
    deserializer.parseIndexTable();
    
    // Try loading a non-existent function
    auto nullFunc = deserializer.loadFunction(99);
    assert(nullFunc == nullptr);
    
    // Load the correct function
    auto loadedFunc = deserializer.loadFunction(17);
    assert(loadedFunc != nullptr);
    assert(loadedFunc->blocks.size() == 1);
    assert(loadedFunc->blocks[0]->label.name == "bb0");
    assert(loadedFunc->blocks[0]->instructions.size() == 1);
    
    assert(loadedFunc->blocks[0]->instructions[0]->getOpcode() == Opcode::Alu);
    assert(loadedFunc->blocks[0]->terminator->getOpcode() == Opcode::Ret);
}

void testObjectCodeSection() {
    // Mock object blobs for 3 functions
    const uint8_t blob1[] = {0x48, 0x89, 0xC3, 0x90};        // func 1
    const uint8_t blob2[] = {0x55, 0x48, 0x8B, 0xEC, 0xC3};  // func 2
    const uint8_t blob3[] = {0xC3};                            // func 3

    ObjectCodeBuilder builder;
    builder.addFunction(10, blob1, sizeof(blob1));
    builder.addFunction(20, blob2, sizeof(blob2));
    builder.addFunction(30, blob3, sizeof(blob3));

    assert(builder.getFunctionCount() == 3);

    BinaryWriter writer;
    builder.serialize(writer);
    auto sectionBytes = writer.takeBuffer();

    ObjectCodeExtractor extractor(sectionBytes.data(), sectionBytes.size());
    extractor.parseIndexTable();

    assert(extractor.getFunctionCount() == 3);

    // Test: extracting non-existent function
    auto missing = extractor.extractFunction(99);
    assert(!missing.has_value());

    // Test: Lazy loading — extract only function 20
    auto func2 = extractor.extractFunction(20);
    assert(func2.has_value());
    assert(func2->size() == sizeof(blob2));
    assert(std::memcmp(func2->data(), blob2, sizeof(blob2)) == 0);

    // Test: function 10 and 30 untouched, extract correctly
    auto func1 = extractor.extractFunction(10);
    assert(func1.has_value());
    assert(std::memcmp(func1->data(), blob1, sizeof(blob1)) == 0);

    auto func3 = extractor.extractFunction(30);
    assert(func3.has_value());
    assert(func3->data()[0] == 0xC3);

    // Test: getHash is consistent
    auto hash20a = extractor.getHash(20);
    assert(hash20a.has_value());

    // Test: extractAll() returns concatenated blobs in order
    auto all = extractor.extractAll();
    assert(all.size() == sizeof(blob1) + sizeof(blob2) + sizeof(blob3));
    assert(std::memcmp(all.data(), blob1, sizeof(blob1)) == 0);
    assert(std::memcmp(all.data() + sizeof(blob1), blob2, sizeof(blob2)) == 0);
    assert(all.back() == 0xC3);
}

void testDebugSection() {
    DebugInfoBuilder builder;
    // Function 5, instruction 0: src/main.mel line 10, col 4
    builder.addLineEntry(5, 0, /*fileStringID=*/100, 10, 4);
    // Function 5, instruction 1: same file, line 11, col 8
    builder.addLineEntry(5, 1, 100, 11, 8);
    // Function 7, instruction 0: different function
    builder.addLineEntry(7, 0, 101, 42, 1);

    assert(builder.getEntryCount() == 3);

    BinaryWriter writer;
    builder.serialize(writer);
    auto bytes = writer.takeBuffer();

    DebugInfoReader reader(bytes.data(), bytes.size());
    reader.parse();
    assert(reader.getEntryCount() == 3);

    auto loc = reader.lookup(5, 1);
    assert(loc.has_value());
    assert(loc->line == 11);
    assert(loc->column == 8);
    assert(loc->fileStringID == 100);

    auto missing = reader.lookup(5, 99);
    assert(!missing.has_value());
}

void testDepGraph() {
    DepGraphBuilder builder;
    // A(1) calls B(2), B(2) calls C(3), D(4) calls C(3)
    builder.addEdge(1, 2); // A -> B
    builder.addEdge(2, 3); // B -> C
    builder.addEdge(4, 3); // D -> C

    assert(builder.getEdgeCount() == 3);

    BinaryWriter writer;
    builder.serialize(writer);
    auto bytes = writer.takeBuffer();

    DepGraphReader reader(bytes.data(), bytes.size());
    reader.parse();
    assert(reader.getEdgeCount() == 3);

    // When C(3) changes, dependents are B(2), A(1), and D(4)
    auto dependents = reader.getDependents(3);
    assert(dependents.count(2) == 1); // B depends on C
    assert(dependents.count(1) == 1); // A depends on C transitively via B
    assert(dependents.count(4) == 1); // D directly depends on C
    assert(dependents.count(3) == 0); // C itself is not in the set
    assert(dependents.size() == 3);

    // When B(2) changes, only A is affected
    auto bDeps = reader.getDependents(2);
    assert(bDeps.count(1) == 1);
    assert(bDeps.count(4) == 0);
}

void testDocSection() {
    StringTableBuilder stringTable;
    DocSectionBuilder builder(stringTable);

    builder.addDoc(10, "Computes the maximum of two values.");
    builder.addDoc(20, "A generic Vec container.");
    builder.addDoc(10, "Overloaded doc."); // same ID — both stored, reader returns last

    BinaryWriter docWriter;
    builder.serialize(docWriter);
    auto docBytes = docWriter.takeBuffer();

    BinaryWriter strWriter;
    stringTable.serialize(strWriter);
    auto strBytes = strWriter.takeBuffer();

    DocSectionReader reader(
        docBytes.data(), docBytes.size(),
        strBytes.data(), strBytes.size()
    );
    reader.parse();

    auto doc20 = reader.getDoc(20);
    assert(doc20.has_value());
    assert(*doc20 == "A generic Vec container.");

    auto missing = reader.getDoc(99);
    assert(!missing.has_value());
}

void testMacroMetadata() {
    MacroMetadataBuilder builder;
    builder.addMacro("vec", "macro vec(@items: expr...) { dec v = $crate::Vec::new(); return v; }");

    BinaryWriter writer;
    builder.serialize(writer);
    auto bytes = writer.takeBuffer();

    BinaryReader reader(bytes.data(), bytes.size());
    uint32_t count = reader.readU32();
    assert(count == 1);
    
    std::string name = reader.readString();
    std::string src = reader.readString();
    assert(name == "vec");
    assert(src.find("$crate") != std::string::npos);
}

#include "mellis/MLib/GenericMetadataBuilder.h"
void testGenericMetadata() {
    GenericMetadataBuilder builder;
    builder.addGeneric(fl::mlib::GenericKind::Function, "my_generic_fn", "fn my_generic_fn<T>(a: T) {}");
    builder.addGeneric(fl::mlib::GenericKind::Impl, "std::Vec", "impl<T> std::Vec<T> {}");

    BinaryWriter writer;
    builder.serialize(writer);
    auto bytes = writer.takeBuffer();

    BinaryReader reader(bytes.data(), bytes.size());
    uint32_t count = reader.readU32();
    std::cout << "Count: " << count << std::endl;
    assert(count == 2);
    
    // First
    auto kind1 = reader.readU8();
    std::cout << "Kind1: " << (int)kind1 << std::endl;
    assert(kind1 == static_cast<uint8_t>(fl::mlib::GenericKind::Function));
    std::string name1 = reader.readString();
    std::cout << "Name1: " << name1 << std::endl;
    assert(name1 == "my_generic_fn");
    std::string src1 = reader.readString();
    std::cout << "Src1: " << src1 << std::endl;
    assert(src1 == "fn my_generic_fn<T>(a: T) {}");
    
    // Second
    auto kind2 = reader.readU8();
    std::cout << "Kind2: " << (int)kind2 << std::endl;
    assert(kind2 == static_cast<uint8_t>(fl::mlib::GenericKind::Impl));
    std::string name2 = reader.readString();
    std::cout << "Name2: " << name2 << std::endl;
    assert(name2 == "std::Vec");
    std::string src2 = reader.readString();
    std::cout << "Src2: " << src2 << std::endl;
    assert(src2 == "impl<T> std::Vec<T> {}");
    std::cout << "testGenericMetadata passed\n";
}

int main() {
    testBasicWriterReader();
    std::cout << "testBasicWriterReader passed\n";

    testStringTableDeduplication();
    std::cout << "testStringTableDeduplication passed\n";

    testManifestAndDependency();
    std::cout << "testManifestAndDependency passed\n";

    testMetadataArena();
    std::cout << "testMetadataArena passed\n";

    testGenericSectionIndex();
    std::cout << "testGenericSectionIndex passed\n";

    testObjectCodeSection();
    std::cout << "testObjectCodeSection passed\n";

    testDebugSection();
    std::cout << "testDebugSection passed\n";

    testDepGraph();
    testDocSection();
    testMacroMetadata();
    testGenericMetadata();
    
    std::cout << "All MLib core tests passed!" << std::endl;
    return 0;
}
