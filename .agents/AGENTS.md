
# Mellis Compiler Architecture Rules

1. Every phase owns exactly one responsibility.
2. Every phase communicates only through stable IR.
3. No phase may inspect internal data of another phase.
4. Information only flows forward. Backward dependencies are forbidden.
5. Every intermediate representation has a single owner.
   - AST      -> FrontEnd
   - Semantic -> MiddleEnd
   - MVIR     -> Optimizer
   - LLVM IR  -> Backend

