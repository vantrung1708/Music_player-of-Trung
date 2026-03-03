#include "playback/VideoEngine.hpp"
#include <chrono>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace playback {

static double clampRate_(double rate) {
  if (rate < 0.25) return 0.25;
  if (rate > 4.0) return 4.0;
  return rate;
}

VideoEngine::VideoEngine() = default;

VideoEngine::~VideoEngine() {
  stop();
}

bool VideoEngine::open(const std::string& path) {
  std::lock_guard<std::mutex> lock(decodeMtx_);
  closeFFmpeg_();
  if (!openFFmpeg_(path)) {
    return false;
  }
  {
    std::lock_guard<std::mutex> syncLock(syncMtx_);
    clockInit_ = false;
    frameIndex_ = 0;
  }
  syncReset_ = true;
  paused_ = false;
  if (!running_) {
    running_ = true;
    decodeThread_ = std::thread(&VideoEngine::decodeThreadMain_, this);
  }
  return true;
}

void VideoEngine::play() {
  if (playing_) {
    return;
  }
  auto now = std::chrono::steady_clock::now();
  if (paused_.exchange(false)) {
    std::lock_guard<std::mutex> syncLock(syncMtx_);
    if (clockInit_) {
      baseTime_ += (now - pauseTime_);
    }
  }
  playing_ = true;
}

void VideoEngine::pause() {
  if (!playing_) {
    return;
  }
  playing_ = false;
  paused_ = true;
  pauseTime_ = std::chrono::steady_clock::now();
}

void VideoEngine::stop() {
  playing_ = false;
  paused_ = false;
  running_ = false;
  if (decodeThread_.joinable()) {
    decodeThread_.join();
  }
  closeFFmpeg_();
  std::lock_guard<std::mutex> syncLock(syncMtx_);
  clockInit_ = false;
  frameIndex_ = 0;
}

bool VideoEngine::seek(int64_t targetMs) {
  std::lock_guard<std::mutex> lock(decodeMtx_);
  if (!fmt_ || !dec_ || videoStreamIndex_ < 0 || timeBase_ <= 0.0) {
    return false;
  }
  const int64_t ts = static_cast<int64_t>((targetMs / 1000.0) / timeBase_);
  if (av_seek_frame(fmt_, videoStreamIndex_, ts, AVSEEK_FLAG_BACKWARD) < 0) {
    return false;
  }
  avcodec_flush_buffers(dec_);
  {
    std::lock_guard<std::mutex> syncLock(syncMtx_);
    clockInit_ = false;
    frameIndex_ = 0;
  }
  syncReset_ = true;
  return true;
}

void VideoEngine::setPlaybackRate(double rate) {
  playbackRate_ = clampRate_(rate);
  syncReset_ = true;
}

double VideoEngine::playbackRate() const {
  return playbackRate_.load();
}

void VideoEngine::decodeThreadMain_() {
  AVPacket* pkt = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  const auto maxLag = std::chrono::milliseconds(200);

  while (running_) {
    if (!playing_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    if (syncReset_.exchange(false)) {
      std::lock_guard<std::mutex> syncLock(syncMtx_);
      clockInit_ = false;
      frameIndex_ = 0;
    }

    std::unique_lock<std::mutex> lock(decodeMtx_);
    if (!fmt_ || !dec_) {
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    int ret = av_read_frame(fmt_, pkt);
    if (ret == AVERROR_EOF) {
      lock.unlock();
      playing_ = false;
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      continue;
    }
    if (ret < 0) {
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    if (pkt->stream_index != videoStreamIndex_) {
      av_packet_unref(pkt);
      lock.unlock();
      continue;
    }

    ret = avcodec_send_packet(dec_, pkt);
    av_packet_unref(pkt);
    if (ret < 0) {
      lock.unlock();
      continue;
    }

    while (ret >= 0) {
      ret = avcodec_receive_frame(dec_, frame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      }
      if (ret < 0) {
        break;
      }

      const double rate = clampRate_(playbackRate_.load());
      int64_t pts = (frame->pts == AV_NOPTS_VALUE) ? frame->best_effort_timestamp : frame->pts;
      bool displayed = true;

      if (pts != AV_NOPTS_VALUE && timeBase_ > 0.0) {
        std::chrono::steady_clock::time_point baseTime;
        int64_t basePts = 0;
        {
          std::lock_guard<std::mutex> syncLock(syncMtx_);
          if (!clockInit_) {
            baseTime_ = std::chrono::steady_clock::now();
            basePts_ = pts;
            clockInit_ = true;
            frameIndex_ = 0;
          }
          baseTime = baseTime_;
          basePts = basePts_;
        }

        const double relSec = (pts - basePts) * timeBase_;
        const auto target = baseTime + std::chrono::milliseconds(static_cast<int64_t>(relSec * 1000.0 / rate));
        const auto now = std::chrono::steady_clock::now();
        if (target > now) {
          std::this_thread::sleep_until(target);
        } else if (now - target > maxLag) {
          displayed = false;
        }
      } else if (fpsNum_ > 0 && fpsDen_ > 0) {
        const double frameSec = static_cast<double>(fpsDen_) / fpsNum_;
        std::chrono::steady_clock::time_point baseTime;
        int64_t frameIndex = 0;
        {
          std::lock_guard<std::mutex> syncLock(syncMtx_);
          if (!clockInit_) {
            baseTime_ = std::chrono::steady_clock::now();
            clockInit_ = true;
            frameIndex_ = 0;
          }
          frameIndex = frameIndex_++;
          baseTime = baseTime_;
        }

        const auto target = baseTime + std::chrono::milliseconds(static_cast<int64_t>((frameIndex * frameSec * 1000.0) / rate));
        const auto now = std::chrono::steady_clock::now();
        if (target > now) {
          std::this_thread::sleep_until(target);
        } else if (now - target > maxLag) {
          displayed = false;
        }
      }

      if (!displayed) {
        av_frame_unref(frame);
        continue;
      }

      VideoFrame out;
      out.width = outWidth_;
      out.height = outHeight_;
      out.rgba.resize(static_cast<size_t>(outWidth_ * outHeight_ * 4));
      uint8_t* dstData[4] = { out.rgba.data(), nullptr, nullptr, nullptr };
      int dstLinesize[4] = { outWidth_ * 4, 0, 0, 0 };

      sws_scale(sws_, frame->data, frame->linesize, 0, dec_->height, dstData, dstLinesize);

      if (frameCb_) {
        frameCb_(out);
      }

      av_frame_unref(frame);
    }
    lock.unlock();
  }

  av_frame_free(&frame);
  av_packet_free(&pkt);
}

bool VideoEngine::openFFmpeg_(const std::string& path) {
  auto fail = [this]() {
    closeFFmpeg_();
    return false;
  };

  if (avformat_open_input(&fmt_, path.c_str(), nullptr, nullptr) < 0) {
    return fail();
  }
  if (avformat_find_stream_info(fmt_, nullptr) < 0) {
    return fail();
  }

  videoStreamIndex_ = -1;
  for (unsigned int i = 0; i < fmt_->nb_streams; ++i) {
    if (fmt_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStreamIndex_ = static_cast<int>(i);
      break;
    }
  }
  if (videoStreamIndex_ < 0) {
    return fail();
  }

  AVStream* stream = fmt_->streams[videoStreamIndex_];
  const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!codec) {
    return fail();
  }

  dec_ = avcodec_alloc_context3(codec);
  if (!dec_) {
    return fail();
  }
  if (avcodec_parameters_to_context(dec_, stream->codecpar) < 0) {
    return fail();
  }
  if (avcodec_open2(dec_, codec, nullptr) < 0) {
    return fail();
  }

  outWidth_ = dec_->width;
  outHeight_ = dec_->height;
  timeBase_ = av_q2d(stream->time_base);
  fpsNum_ = stream->avg_frame_rate.num;
  fpsDen_ = stream->avg_frame_rate.den;

  sws_ = sws_getContext(dec_->width, dec_->height, dec_->pix_fmt,
                        outWidth_, outHeight_, AV_PIX_FMT_RGBA,
                        SWS_BILINEAR, nullptr, nullptr, nullptr);
  if (!sws_) {
    return fail();
  }

  return true;
}

void VideoEngine::closeFFmpeg_() {
  if (dec_) {
    avcodec_free_context(&dec_);
    dec_ = nullptr;
  }
  if (fmt_) {
    avformat_close_input(&fmt_);
    fmt_ = nullptr;
  }
  if (sws_) {
    sws_freeContext(sws_);
    sws_ = nullptr;
  }
  videoStreamIndex_ = -1;
  timeBase_ = 0.0;
  fpsNum_ = 0;
  fpsDen_ = 0;
  outWidth_ = 0;
  outHeight_ = 0;
}

} // namespace playback
