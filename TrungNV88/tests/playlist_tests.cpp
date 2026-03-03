#include <gtest/gtest.h>
#include "model/PlaylistManager.hpp"
#include <algorithm>
#include <filesystem>
#include <string>

TEST(PlaylistManagerTests, SaveLoadRoundTrip) {
  model::PlaylistManager pm;
  std::vector<core::Track> tracks;
  core::Track a;
  a.filePath = "song-a.mp3";
  a.duration = 1234;
  a.meta.title = "A";
  tracks.push_back(a);
  core::Track b;
  b.filePath = "song-b.mp3";
  b.duration = 5678;
  b.meta.title = "B";
  tracks.push_back(b);

  pm.setTracks(tracks);

  auto path = std::filesystem::temp_directory_path() / "s32_playlist_test.json";
  ASSERT_TRUE(pm.save(path));

  model::PlaylistManager pm2;
  ASSERT_TRUE(pm2.load(path));
  ASSERT_EQ(pm2.tracks().size(), 2u);
  EXPECT_EQ(pm2.tracks()[0].filePath, "song-a.mp3");
  EXPECT_EQ(pm2.tracks()[1].filePath, "song-b.mp3");

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(PlaylistManagerTests, ShuffleKeepsElements) {
  model::PlaylistManager pm;
  std::vector<core::Track> tracks;
  for (int i = 0; i < 10; ++i) {
    core::Track t;
    t.filePath = "track-" + std::to_string(i);
    tracks.push_back(t);
  }
  pm.setTracks(tracks);
  pm.shuffle();

  ASSERT_EQ(pm.tracks().size(), tracks.size());
  std::vector<std::string> original;
  std::vector<std::string> shuffled;
  for (const auto& t : tracks) {
    original.push_back(t.filePath);
  }
  for (const auto& t : pm.tracks()) {
    shuffled.push_back(t.filePath);
  }
  std::sort(original.begin(), original.end());
  std::sort(shuffled.begin(), shuffled.end());
  EXPECT_EQ(original, shuffled);
}
