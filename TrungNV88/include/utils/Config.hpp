#pragma once
#include <filesystem>

namespace utils {

std::filesystem::path configDir();
std::filesystem::path playlistPath();
std::filesystem::path settingsPath();

} // namespace utils
