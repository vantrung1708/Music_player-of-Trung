#include "controller/VideoController.hpp"
#include "view/VideoFrameProvider.hpp"
#include <QImage>
#include <QMetaObject>
#include <string>

namespace controller {

VideoController::VideoController(VideoFrameProvider* provider, QObject* parent)
  : QObject(parent), provider_(provider) {
  engine_.setFrameCallback([this](const playback::VideoFrame& frame) {
    onFrame_(frame);
  });
  engine_.setPlaybackRate(playbackRate_);
}

int VideoController::frameCounter() const {
  return frameCounter_;
}

bool VideoController::active() const {
  return active_;
}

double VideoController::playbackRate() const {
  return playbackRate_;
}

void VideoController::openAndPlay(const QString& path) {
  const std::string p = path.toStdString();
  if (p.empty()) {
    return;
  }
  engine_.stop();
  engine_.setPlaybackRate(playbackRate_);
  if (engine_.open(p)) {
    active_ = true;
    emit activeChanged();
    engine_.play();
  } else {
    active_ = false;
    emit activeChanged();
  }
}

void VideoController::play() {
  if (!active_) {
    return;
  }
  engine_.play();
}

void VideoController::pause() {
  engine_.pause();
}

void VideoController::seek(qint64 posMs) {
  if (!active_) {
    return;
  }
  engine_.seek(posMs);
}

void VideoController::stop() {
  engine_.stop();
  active_ = false;
  emit activeChanged();
}

void VideoController::setPlaybackRate(double rate) {
  if (playbackRate_ == rate) {
    return;
  }
  playbackRate_ = rate;
  engine_.setPlaybackRate(rate);
  emit playbackRateChanged();
}

void VideoController::onFrame_(const playback::VideoFrame& frame) {
  if (!provider_ || frame.rgba.empty() || frame.width <= 0 || frame.height <= 0) {
    return;
  }
  QImage img(frame.rgba.data(), frame.width, frame.height, QImage::Format_RGBA8888);
  provider_->setFrame(img);

  QMetaObject::invokeMethod(this, [this]() {
    ++frameCounter_;
    emit frameCounterChanged();
  }, Qt::QueuedConnection);
}

} // namespace controller
