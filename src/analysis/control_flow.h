#pragma once
// Planned: CFG / basic-block recovery for PPU images.
// Today function discovery remains inside the lifter; this header defines the
// future analysis boundary so GUI/CLI can call a stable API later.
#include <cstdint>
#include <vector>

namespace ppsx33 {
namespace analysis {

struct BasicBlock {
    uint64_t start = 0;
    uint64_t end = 0;
    std::vector<uint64_t> successors;
};

struct FunctionExtent {
    uint64_t entry = 0;
    uint64_t toc = 0;
    std::vector<BasicBlock> blocks;
};

// Placeholder — returns empty until CFG recovery is implemented.
inline bool discover_functions_stub(const uint8_t* /*image*/, size_t /*size*/,
                                    std::vector<FunctionExtent>& out) {
    out.clear();
    return false;
}

} // namespace analysis
} // namespace ppsx33
