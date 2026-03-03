#include "model/PlaylistManager.hpp"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>

namespace model {

void PlaylistManager::setTracks(std::vector<core::Track> tracks) {
  tracks_ = std::move(tracks);
  currentIndex_ = 0;
}

const std::vector<core::Track>& PlaylistManager::tracks() const {
  return tracks_;
}

const core::Track* PlaylistManager::current() const {
  if (tracks_.empty()) {
    return nullptr;
  }
  if (currentIndex_ >= tracks_.size()) {
    return nullptr;
  }
  return &tracks_[currentIndex_];
}

bool PlaylistManager::next() {
  if (currentIndex_ + 1 < tracks_.size()) {
    ++currentIndex_;
    return true;
  }
  return false;
}

bool PlaylistManager::hasNext() const {
  return currentIndex_ + 1 < tracks_.size();
}

void PlaylistManager::reset() {
  currentIndex_ = 0;
}

void PlaylistManager::shuffle() {
  if (tracks_.empty()) {
    return;
  }
  std::shuffle(tracks_.begin(), tracks_.end(), rng_);
  currentIndex_ = 0;
}

bool PlaylistManager::save(const std::filesystem::path& path) const {
  try {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& t : tracks_) {
      nlohmann::json item;
      item["path"] = t.filePath;
      item["duration"] = t.duration;
      item["title"] = t.meta.title;
      item["artist"] = t.meta.artist;
      item["album"] = t.meta.album;
      item["coverArtPath"] = t.meta.coverArtPath;
      j.push_back(item);
    }

    std::ofstream out(path);
    if (!out) {
      return false;
    }
    out << j.dump(2);
    return true;
  } catch (...) {
    return false;
  }
}

bool PlaylistManager::load(const std::filesystem::path& path) {
  try {
    std::ifstream in(path);
    if (!in) {
      return false;
    }
    nlohmann::json j;
    in >> j;
    if (!j.is_array()) {
      return false;
    }

    std::vector<core::Track> loaded;
    for (const auto& item : j) {
      core::Track t;
      t.filePath = item.value("path", "");
      t.duration = item.value("duration", 0);
      t.meta.title = item.value("title", "");
      t.meta.artist = item.value("artist", "");
      t.meta.album = item.value("album", "");
      t.meta.coverArtPath = item.value("coverArtPath", "");
      if (!t.filePath.empty()) {
        loaded.push_back(std::move(t));
      }
    }
    tracks_ = std::move(loaded);
    currentIndex_ = 0;
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace model
