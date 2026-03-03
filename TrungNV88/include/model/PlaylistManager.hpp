#pragma once
#include <filesystem>
#include <random>
#include <vector>
#include "core/ITrack.hpp"

namespace model {

class PlaylistManager {
public:
  void setTracks(std::vector<core::Track> tracks);
  const std::vector<core::Track>& tracks() const;

  const core::Track* current() const;
  bool next();
  bool hasNext() const;
  void reset();

  void shuffle();

  bool save(const std::filesystem::path& path) const;
  bool load(const std::filesystem::path& path);

private:
  std::vector<core::Track> tracks_;
  size_t currentIndex_ = 0;
  std::mt19937 rng_{std::random_device{}()};
};

} // namespace model
