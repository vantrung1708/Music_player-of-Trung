#include "utils/Config.hpp"
#include <cstdlib>
#include <system_error>

namespace utils {

static std::filesystem::path resolveHomeConfig_() {
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  if (xdg && *xdg) {
    return std::filesystem::path(xdg);
  }
  const char* home = std::getenv("HOME");
  if (home && *home) {
    return std::filesystem::path(home) / ".config";
  }
  return std::filesystem::temp_directory_path();
}

std::filesystem::path configDir() {
  auto dir = resolveHomeConfig_() / "s32-media-hub";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  return dir;
}

std::filesystem::path playlistPath() {
  return configDir() / "playlist.json";
}

std::filesystem::path settingsPath() {
  return configDir() / "settings.json";
}

} // namespace utils
