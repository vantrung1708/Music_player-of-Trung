#include "view/VideoFrameProvider.hpp"

VideoFrameProvider::VideoFrameProvider()
  : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage VideoFrameProvider::requestImage(const QString&, QSize* size, const QSize& requestedSize) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (size) {
    *size = frame_.size();
  }
  if (frame_.isNull()) {
    return QImage();
  }
  if (requestedSize.isValid()) {
    return frame_.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }
  return frame_;
}

void VideoFrameProvider::setFrame(const QImage& img) {
  std::lock_guard<std::mutex> lock(mtx_);
  frame_ = img.copy();
}
