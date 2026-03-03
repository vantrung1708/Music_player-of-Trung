#include "model/LibraryScanner.hpp"
#include <algorithm>
#include <cctype>

namespace model {

std::vector<core::Track> LibraryScanner::scan(const std::filesystem::path& root,
                                               ProgressCallback cb) {
  std::vector<core::Track> results;
  if (!std::filesystem::exists(root)) {
    return results;
  }

  size_t scanned = 0;
  std::error_code ec;
  for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
       it != std::filesystem::recursive_directory_iterator();
       it.increment(ec)) {
    if (ec) {
      continue;
    }
    if (!it->is_regular_file()) {
      continue;
    }
    const auto& path = it->path();
    if (!isSupportedExtension(path)) {
      continue;
    }
    core::Track t;
    t.filePath = path.string();
    results.push_back(std::move(t));
    ++scanned;
    if (cb) {
      cb(scanned);
    }
  }

  return results;
}

static std::string toLower_(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

bool LibraryScanner::isSupportedExtension(const std::filesystem::path& path) {
  const auto ext = toLower_(path.extension().string());
  return ext == ".mp3" || ext == ".flac" || ext == ".mkv" || ext == ".wav";
}

} // namespace model
