#include "controller/PlaybackController.hpp"
#include <QMetaObject>
#include <QFileInfo>

namespace controller {

PlaybackController::PlaybackController(std::shared_ptr<model::PlaylistManager> playlist,
                                       QObject* parent)
  : QObject(parent), playlist_(std::move(playlist)) {
  engine_.init();
  bindEngineCallbacks_();
}

bool PlaybackController::playing() const {
  return playing_;
}

qint64 PlaybackController::positionMs() const {
  return positionMs_;
}

qint64 PlaybackController::durationMs() const {
  return durationMs_;
}

float PlaybackController::volume() const {
  return volume_;
}

QString PlaybackController::currentPath() const {
  return currentPath_;
}

QString PlaybackController::currentTitle() const {
  return currentTitle_;
}

QString PlaybackController::currentArtist() const {
  return currentArtist_;
}

void PlaybackController::playPause() {
  if (playing_) {
    engine_.pause();
    return;
  }

  if (currentPath_.isEmpty() && playlist_ && !playlist_->tracks().empty()) {
    const auto& t = playlist_->tracks().front();
    openAndPlay(QString::fromStdString(t.filePath));
    return;
  }

  engine_.play();
}

void PlaybackController::openAndPlay(const QString& path) {
  const std::string p = path.toStdString();
  if (p.empty()) {
    return;
  }
  if (engine_.open(p)) {
    currentPath_ = path;
    emit currentPathChanged();
    updateCurrentMeta_();
    updateDuration_();
    engine_.play();
  }
}

void PlaybackController::stop() {
  engine_.stop();
}

void PlaybackController::seek(qint64 posMs) {
  engine_.seek(posMs);
}

void PlaybackController::next() {
  if (!playlist_) {
    return;
  }
  const auto& tracks = playlist_->tracks();
  if (tracks.empty()) {
    return;
  }
  size_t idx = 0;
  bool found = false;
  for (size_t i = 0; i < tracks.size(); ++i) {
    if (tracks[i].filePath == currentPath_.toStdString()) {
      idx = i;
      found = true;
      break;
    }
  }
  if (!found) {
    idx = 0;
  }
  if (idx + 1 < tracks.size()) {
    const auto& nextTrack = tracks[idx + 1];
    openAndPlay(QString::fromStdString(nextTrack.filePath));
  }
}

void PlaybackController::shuffle() {
  if (playlist_) {
    playlist_->shuffle();
  }
}

void PlaybackController::setVolume(float v01) {
  volume_ = v01;
  engine_.setVolume(v01);
  emit volumeChanged();
}

void PlaybackController::bindEngineCallbacks_() {
  engine_.onStateChanged([this](bool playing) {
    QMetaObject::invokeMethod(this, [this, playing]() {
      playing_ = playing;
      emit playingChanged();
    }, Qt::QueuedConnection);
  });

  engine_.onTime([this](int64_t posMs, int64_t durMs) {
    QMetaObject::invokeMethod(this, [this, posMs, durMs]() {
      positionMs_ = static_cast<qint64>(posMs);
      durationMs_ = static_cast<qint64>(durMs);
      emit positionChanged();
      emit durationChanged();
    }, Qt::QueuedConnection);
  });

  engine_.onEnd([this]() {
    QMetaObject::invokeMethod(this, [this]() {
      playing_ = false;
      emit playingChanged();
    }, Qt::QueuedConnection);
  });
}

void PlaybackController::updateDuration_() {
  durationMs_ = static_cast<qint64>(engine_.durationMs());
  emit durationChanged();
}

void PlaybackController::updateCurrentMeta_() {
  QString title;
  QString artist;
  if (playlist_) {
    const auto& tracks = playlist_->tracks();
    const auto currentStd = currentPath_.toStdString();
    for (const auto& t : tracks) {
      if (t.filePath == currentStd) {
        title = QString::fromStdString(t.meta.title);
        artist = QString::fromStdString(t.meta.artist);
        break;
      }
    }
  }

  if (title.isEmpty() && !currentPath_.isEmpty()) {
    title = QFileInfo(currentPath_).fileName();
  }

  if (title != currentTitle_) {
    currentTitle_ = title;
    emit currentTitleChanged();
  }
  if (artist != currentArtist_) {
    currentArtist_ = artist;
    emit currentArtistChanged();
  }
}

} // namespace controller
