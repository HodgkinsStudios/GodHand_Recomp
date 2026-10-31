#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace godhand {

using ProgressCallback = std::function<void(const std::string&, double)>;

struct InstallResult {
    bool ok = false;
    std::string message;
};

std::filesystem::path data_directory();
bool is_installed(const std::filesystem::path& data_dir);
bool sanitized_iso_path(const std::string& raw_path, std::filesystem::path& relative);
InstallResult inspect_iso(const std::filesystem::path& source);
InstallResult install_iso(const std::filesystem::path& source,
                          const std::filesystem::path& data_dir,
                          const ProgressCallback& progress = {});

} // namespace godhand
