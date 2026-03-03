#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace playback {

struct VideoFrame {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> rgba;
};

class VideoEngine {
public:
  using FrameCallback = std::function<void(const VideoFrame&)>;

  VideoEngine();
  ~VideoEngine();

  bool open(const std::string& path);
  void play();
  void pause();
  void stop();
  bool seek(int64_t targetMs);
  void setPlaybackRate(double rate);
  double playbackRate() const;

  void setFrameCallback(FrameCallback cb) { frameCb_ = std::move(cb); }

private:
  void decodeThreadMain_();
  bool openFFmpeg_(const std::string& path);
  void closeFFmpeg_();

private:
  std::atomic<bool> running_{false};
  std::atomic<bool> playing_{false};
  std::atomic<bool> syncReset_{false};
  std::atomic<bool> paused_{false};
  std::atomic<double> playbackRate_{1.0};
  std::thread decodeThread_;
  std::mutex decodeMtx_;
  std::mutex syncMtx_;

  FrameCallback frameCb_;

  struct AVFormatContext* fmt_ = nullptr;
  struct AVCodecContext*  dec_ = nullptr;
  struct SwsContext*      sws_ = nullptr;
  int videoStreamIndex_ = -1;
  double timeBase_ = 0.0;
  int fpsNum_ = 0;
  int fpsDen_ = 0;

  int outWidth_ = 0;
  int outHeight_ = 0;

  bool clockInit_ = false;
  int64_t basePts_ = 0;
  int64_t frameIndex_ = 0;
  std::chrono::steady_clock::time_point baseTime_;
  std::chrono::steady_clock::time_point pauseTime_;
};

} // namespace playback
