#pragma once
#include "installer.hpp"
#include <filesystem>

namespace godhand {
// Validates the AFS header, entry table, completion marker and all extracted
// regular-file sizes. Does not read 1.5 GiB of asset payload on every launch.
bool afs_extraction_complete(const std::filesystem::path& data_dir);
// Streams genuine AFS entry bytes to local files; repairs missing/incomplete
// entries and publishes the completion marker only after full verification.
InstallResult ensure_afs_extracted(const std::filesystem::path& data_dir,
                                  const ProgressCallback& progress = {});
} // namespace godhand
