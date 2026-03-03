#pragma once
#include <cstdint>
#include <string>

namespace core {

struct TrackMetadata {
  std::string title;
  std::string artist;
  std::string album;
  std::string coverArtPath;
};

class ITrack {
public:
  virtual ~ITrack() = default;
  virtual const std::string& path() const = 0;
  virtual int64_t durationMs() const = 0;
  virtual const TrackMetadata& metadata() const = 0;
};

struct Track final : public ITrack {
  std::string filePath;
  int64_t duration = 0;
  TrackMetadata meta;

  const std::string& path() const override { return filePath; }
  int64_t durationMs() const override { return duration; }
  const TrackMetadata& metadata() const override { return meta; }
};

} // namespace core
