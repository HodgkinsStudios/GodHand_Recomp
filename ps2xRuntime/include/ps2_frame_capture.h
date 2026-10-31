#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace ps2x
{
    // Optional single-shot diagnostic. Never fabricates a host-visible image;
    // accepts only decoded RGBA pixels from the guest GS presentation backend.
    // A positive result means non-uniform guest pixels were saved, not that
    // gameplay, draw-call correctness, or fidelity has been established.
    bool captureNonUniformGuestFrame(const std::vector<uint8_t> &rgba,
                                     uint32_t width, uint32_t height,
                                     const std::filesystem::path &destination);
}
