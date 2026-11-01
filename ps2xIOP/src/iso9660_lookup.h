#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace ps2x::iop {
class IopHost;
namespace detail {
// Resolve a file using the *original* mounted ISO9660 directory and return
// its actual sector number and 32-byte sceCdlFILE descriptor.
// No fallback to synthesized sectors when a physical image is configured.
[[nodiscard]] bool lookupIso9660CdvdFile(
    IopHost& host, std::string_view guestPath, std::array<uint8_t, 32>& out);
}
}
