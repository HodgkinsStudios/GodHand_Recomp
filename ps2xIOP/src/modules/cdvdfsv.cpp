#include "module_factories.h"
#include "../iso9660_lookup.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace ps2x::iop::detail {
namespace {
constexpr uint32_t kSector = 2048u;
constexpr uint32_t kInit = 0x80000592u, kSearch = 0x80000597u, kDiskReady = 0x8000059Au;
constexpr std::array<uint32_t, 3> kSids{kInit, kSearch, kDiskReady};
constexpr std::array<std::string_view, 2> kAliases{"cdvdfsv", "xcdvdfsv"};
uint32_t le32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8u) | (uint32_t(p[2]) << 16u) | (uint32_t(p[3]) << 24u);
}
void put32(uint8_t* p, uint32_t v) {
    for (unsigned i = 0; i < 4u; ++i) p[i] = uint8_t(v >> (8u * i));
}
std::string normalize(std::string_view s) {
    const auto semicolon = s.find(';');
    if (semicolon != std::string_view::npos) s = s.substr(0, semicolon);
    std::string out;
    for (unsigned char c : s) out.push_back(char(std::toupper(c)));
    return out;
}
bool cdPath(std::string_view s, std::vector<std::string>& parts) {
    const auto colon = s.find(':');
    if (colon != std::string_view::npos) {
        const auto device = normalize(s.substr(0, colon));
        if (device != "CDROM" && device != "CDROM0" && device != "CDROM1") return false;
        s.remove_prefix(colon + 1u);
    }
    for (size_t i = 0; i < s.size();) {
        while (i < s.size() && (s[i] == '/' || s[i] == '\\')) ++i;
        const size_t begin = i;
        while (i < s.size() && s[i] != '/' && s[i] != '\\') ++i;
        if (begin == i) continue;
        auto part = normalize(s.substr(begin, i - begin));
        if (part == ".." || part.empty() || part.size() > 222u || parts.size() >= 24u) return false;
        if (part != ".") parts.push_back(std::move(part));
    }
    return !parts.empty();
}
struct FileEntry {
    uint32_t lsn = 0u, size = 0u;
    std::string name;
    std::array<uint8_t, 8> date{};
};
class Disc {
public:
    explicit Disc(IopHost& host) : m_host(host) {
        const auto name = host.hostPath(HostPathKind::CdImage);
        if (!name.empty()) m_handle = host.openHostFile(name);
        if (m_handle) (void)host.hostFileSize(m_handle, m_size);
    }
    ~Disc() { if (m_handle) m_host.closeHostFile(m_handle); }
    Disc(const Disc&) = delete;
    Disc& operator=(const Disc&) = delete;
    bool sector(uint32_t lsn, std::array<uint8_t, kSector>& out) {
        const uint64_t offset = uint64_t(lsn) * kSector;
        if (!m_handle || offset > m_size || m_size - offset < kSector) return false;
        size_t count = 0;
        return m_host.readHostFile(m_handle, offset, out.data(), out.size(), count) && count == out.size();
    }
    bool volume(uint32_t& lsn, uint32_t& size) {
        std::array<uint8_t, kSector> data{};
        if (!sector(16u, data) || data[0] != 1u || std::memcmp(data.data() + 1, "CD001", 5) ||
            data[6] != 1u || data[156] < 34u) return false;
        lsn = le32(data.data() + 158u);
        size = le32(data.data() + 166u);
        return lsn >= 16u && size >= 34u && size <= 8u*1024u*1024u;
    }
    bool find(std::string_view path, FileEntry& output) {
        std::vector<std::string> parts;
        if (!cdPath(path, parts)) return false;
        uint32_t dirLsn = 0u, dirSize = 0u;
        if (!volume(dirLsn, dirSize)) return false;
        for (size_t n = 0; n < parts.size(); ++n) {
            bool found = false, directory = false;
            FileEntry entry{};
            const uint32_t sectors = (dirSize + kSector - 1u) / kSector;
            for (uint32_t s = 0u; s < sectors && !found; ++s) {
                std::array<uint8_t, kSector> bytes{};
                if (!sector(dirLsn + s, bytes)) return false;
                for (uint32_t at = 0u; at + 34u <= kSector;) {
                    const uint32_t len = bytes[at];
                    if (!len) break;
                    if (len < 34u || at + len > kSector) return false;
                    const auto* rec = bytes.data() + at;
                    const uint32_t namelen = rec[32];
                    if (namelen > 1u && 33u + namelen <= len) {
                        const auto name = normalize(std::string_view(reinterpret_cast<const char*>(rec+33u), namelen));
                        if (name == parts[n]) {
                            entry.lsn = le32(rec+2u);
                            entry.size = le32(rec+10u);
                            entry.name = name;
                            std::copy_n(rec+18u, 7u, entry.date.begin());
                            entry.date[7] = rec[25];
                            directory = (rec[25] & 2u) != 0u;
                            found = true;
                            break;
                        }
                    }
                    at += len;
                }
            }
            if (!found || (n + 1u < parts.size() && !directory)) return false;
            if (n + 1u == parts.size()) { output = std::move(entry); return true; }
            if (entry.size < 34u || entry.size > 8u*1024u*1024u) return false;
            dirLsn = entry.lsn;
            dirSize = entry.size;
        }
        return false;
    }
private:
    IopHost& m_host;
    uint64_t m_handle = 0u, m_size = 0u;
};
class Service final : public IopService {
public:
    explicit Service(IopHost& host) : m_host(host) {}
    std::string_view name() const override { return "cdvdfsv"; }
    std::span<const uint32_t> sids() const override { return kSids; }
    std::span<const std::string_view> moduleAliases() const override { return kAliases; }
    void reset() override {}
    RpcResult handleRpc(const RpcRequest& req) override {
        RpcResult reply{};
        if (req.sid != kInit && req.sid != kSearch && req.sid != kDiskReady) return reply;
        reply.handled = true;
        reply.resultAddress = req.receive.address;
        if (req.sid == kInit || req.sid == kDiskReady) {
            Disc disc(m_host);
            uint32_t root = 0u, size = 0u;
            const bool media = disc.volume(root, size);
            std::array<uint8_t, 16> result{};
            put32(result.data(), req.sid == kInit ? (media ? 1u : 0u) : (media ? 2u : 6u));
            const size_t bytes = req.sid == kInit && req.receive.size >= 16u ? 16u : 4u;
            if (req.receive.address && req.receive.size >= bytes)
                (void)m_host.writeGuest(req.receive.address, result.data(), bytes);
            return reply;
        }
        // SearchFilePkt: 32-byte sceCdlFILE, name[256], pointer to output.
        // Never claim a file exists without reading a real ISO9660 entry.
        std::array<uint8_t, 292> packet{};
        bool found = false;
        if (req.function == 0u && req.send.size >= packet.size() && req.send.address &&
            m_host.readGuest(req.send.address, packet.data(), packet.size())) {
            const auto begin = packet.begin() + 32u;
            const auto end = std::find(begin, packet.begin() + 288u, uint8_t(0));
            if (end != packet.begin() + 288u) {
                const std::string path(begin, end);
                std::array<uint8_t, 32> out{};
                if (lookupIso9660CdvdFile(m_host, path, out)) {
                    const auto dest = le32(packet.data()+288u);
                    found = m_host.writeGuest(req.send.address, out.data(), out.size());
                    if (found && dest && dest != req.send.address)
                        found = m_host.writeGuest(dest, out.data(), out.size());
                }
            }
        }
        if (req.receive.address && req.receive.size >= 4u) {
            std::array<uint8_t, 4> out{};
            put32(out.data(), found ? 1u : 0u);
            (void)m_host.writeGuest(req.receive.address, out.data(), out.size());
        }
        return reply;
    }
private:
    IopHost& m_host;
};
} // namespace
bool lookupIso9660CdvdFile(IopHost& host, std::string_view path, std::array<uint8_t, 32>& out) {
    Disc disc(host);
    FileEntry entry{};
    if (!disc.find(path, entry)) return false;
    out.fill(0u);
    put32(out.data(), entry.lsn);
    put32(out.data()+4u, entry.size);
    std::memcpy(out.data()+8u, entry.name.data(), std::min<size_t>(entry.name.size(), 16u));
    std::copy(entry.date.begin(), entry.date.end(), out.begin()+24u);
    return true;
}
std::unique_ptr<IopService> createCdvdfsvService(IopHost& host) {
    return std::make_unique<Service>(host);
}
} // namespace ps2x::iop::detail
