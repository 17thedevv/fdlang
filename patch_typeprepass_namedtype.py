import sys
import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target = '''          void visit(NamedTypeNode& node) override {
              if (node.symbolId != kInvalidSymbolID) {
                  const auto& sym = table.getSymbol(node.symbolId);
                  std::vector<const Type*> args;
                  for (auto& argNode : node.genericArgs) {
                      args.push_back(evaluateTypeNode(argNode.get()));
                  }
                  if (sym.kind == SymbolKind::Struct) evaluatedType = ctx.getStructType(node.symbolId, args);
                  else if (sym.kind == SymbolKind::Enum) evaluatedType = ctx.getEnumType(node.symbolId, args);'''

replacement = '''          void visit(NamedTypeNode& node) override {
              if (node.symbolId != kInvalidSymbolID) {
                  const auto& sym = table.getSymbol(node.symbolId);
                  std::vector<const Type*> args;
                  for (auto& argNode : node.genericArgs) {
                      args.push_back(evaluateTypeNode(argNode.get()));
                  }
                  if (sym.kind == SymbolKind::Struct) {
                      bool allConcrete = true;
                      for (auto* a : args) {
                          if (a->getKind() == TypeKind::InferenceVar || dynamic_cast<const GenericParamType*>(a)) {
                              allConcrete = false; break;
                          }
                      }
                      if (allConcrete && monoEngine && sym.decl) {
                          auto* structDecl = static_cast<const StructDeclNode*>(sym.decl);
                          if (!structDecl->genericParams.empty()) {
                              SymbolID specId = monoEngine->requestStructSpecialization(structDecl, args, node.loc);
                              if (specId != kInvalidSymbolID) {
                                  node.symbolId = specId;
                                  args.clear();
                                  node.genericArgs.clear();
                              }
                          }
                      }
                      evaluatedType = ctx.getStructType(node.symbolId, args);
                  }
                  else if (sym.kind == SymbolKind::Enum) {
                      bool allConcrete = true;
                      for (auto* a : args) {
                          if (a->getKind() == TypeKind::InferenceVar || dynamic_cast<const GenericParamType*>(a)) {
                              allConcrete = false; break;
                          }
                      }
                      if (allConcrete && monoEngine && sym.decl) {
                          auto* enumDecl = static_cast<const EnumDeclNode*>(sym.decl);
                          if (!enumDecl->genericParams.empty()) {
                              SymbolID specId = monoEngine->requestEnumSpecialization(enumDecl, args, node.loc);
                              if (specId != kInvalidSymbolID) {
                                  node.symbolId = specId;
                                  args.clear();
                                  node.genericArgs.clear();
                              }
                          }
                      }
                      evaluatedType = ctx.getEnumType(node.symbolId, args);
                  }'''

if target in content:
    content = content.replace(target, replacement, 1) # Only the one in TypePrePass
    with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
        f.write(content)
    print("Patched TypePrePass NamedTypeNode successfully")
else:
    print("Target not found")
