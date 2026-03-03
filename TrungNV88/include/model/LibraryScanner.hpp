#pragma once
#include <functional>
#include <filesystem>
#include <string>
#include <vector>
#include "core/ITrack.hpp"

namespace model {

class LibraryScanner {
public:
  using ProgressCallback = std::function<void(size_t scanned)>;

  std::vector<core::Track> scan(const std::filesystem::path& root,
                                ProgressCallback cb = {});

  static bool isSupportedExtension(const std::filesystem::path& path);
};

} // namespace model
