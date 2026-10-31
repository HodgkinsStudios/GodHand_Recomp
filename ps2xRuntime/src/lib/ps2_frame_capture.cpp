#include "ps2_frame_capture.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <system_error>

namespace ps2x
{
    bool captureNonUniformGuestFrame(const std::vector<uint8_t> &rgba,
                                     uint32_t width, uint32_t height,
                                     const std::filesystem::path &destination)
    {
        constexpr uint32_t kMaxDimension = 2048;
        if (width < 16 || height < 16 || width > kMaxDimension || height > kMaxDimension ||
            destination.empty())
        {
            return false;
        }

        const uint64_t expected = static_cast<uint64_t>(width) * height * 4u;
        if (expected > rgba.size())
        {
            return false;
        }

        // A black/solid/clear image does not demonstrate that the guest GS
        // actually rendered any visible content. Require distinct sample RGBs.
        const uint32_t stride = std::max(1u, width / 64u);
        uint32_t distinct = 0;
        uint32_t first = 0;
        bool haveFirst = false;
        for (uint32_t y = 0; y < height; y += stride)
        {
            for (uint32_t x = 0; x < width; x += stride)
            {
                const size_t pos = (static_cast<size_t>(y) * width + x) * 4u;
                const uint32_t rgb = (uint32_t(rgba[pos]) << 16u) |
                                     (uint32_t(rgba[pos + 1]) << 8u) | rgba[pos + 2];
                if (!haveFirst)
                {
                    first = rgb;
                    haveFirst = true;
                }
                else if (rgb != first)
                {
                    ++distinct;
                }
                if (distinct >= 8u)
                {
                    break;
                }
            }
            if (distinct >= 8u)
                break;
        }
        if (distinct < 8u)
        {
            return false;
        }

        const auto temporary = destination.string() + ".partial";
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out)
            return false;
        out << "P6\n" << width << ' ' << height << "\n255\n";
        for (size_t i = 0; i < expected; i += 4u)
        {
            out.write(reinterpret_cast<const char *>(rgba.data() + i), 3);
        }
        out.close();
        if (!out)
        {
            std::error_code ec;
            std::filesystem::remove(temporary, ec);
            return false;
        }
        std::error_code ec;
        std::filesystem::rename(temporary, destination, ec);
        if (ec)
        {
            std::filesystem::remove(temporary, ec);
            return false;
        }
        return true;
    }
}
