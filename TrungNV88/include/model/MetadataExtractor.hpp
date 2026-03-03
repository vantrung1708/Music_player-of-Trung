#pragma once
#include <cstdint>
#include <filesystem>
#include "core/ITrack.hpp"

namespace model {

class MetadataExtractor {
public:
  core::TrackMetadata extract(const std::filesystem::path& path) const;
  int64_t durationMs(const std::filesystem::path& path) const;
};

} // namespace model
