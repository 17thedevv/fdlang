#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include "mellis/MiddleEnd/MVIR.h"

namespace fl {

struct LivenessInfo {
    // Map t? Tn Bi?n (Reference) sang t?p cc Instruction pointers m bi?n  cn s?ng (LiveOut)
    std::unordered_map<std::string, std::unordered_set<const mvir::Instruction*>> liveInstructions;
};

class LivenessAnalyzer {
public:
    static LivenessInfo computeLiveness(const mvir::Function& func);
};

} // namespace fl
