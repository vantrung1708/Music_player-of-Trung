#pragma once
#include <QImage>
#include <QQuickImageProvider>
#include <mutex>

class VideoFrameProvider : public QQuickImageProvider {
public:
  VideoFrameProvider();

  QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
  void setFrame(const QImage& img);

private:
  std::mutex mtx_;
  QImage frame_;
};
