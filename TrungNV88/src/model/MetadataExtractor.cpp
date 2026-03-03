#include "model/MetadataExtractor.hpp"
#include <taglib/fileref.h>
#include <taglib/tag.h>

namespace model {

static std::string toStdString_(const TagLib::String& s) {
  auto bv = s.to8Bit(true);
  if (bv.isEmpty()) {
    return {};
  }
  return std::string(bv.data(), bv.size());
}

core::TrackMetadata MetadataExtractor::extract(const std::filesystem::path& path) const {
  core::TrackMetadata meta;
  const auto pathStr = path.string();
  TagLib::FileRef f(pathStr.c_str());
  if (!f.isNull() && f.tag()) {
    TagLib::Tag* tag = f.tag();
    meta.title = toStdString_(tag->title());
    meta.artist = toStdString_(tag->artist());
    meta.album = toStdString_(tag->album());
  }
  return meta;
}

int64_t MetadataExtractor::durationMs(const std::filesystem::path& path) const {
  const auto pathStr = path.string();
  TagLib::FileRef f(pathStr.c_str());
  if (!f.isNull() && f.audioProperties()) {
    return static_cast<int64_t>(f.audioProperties()->lengthInMilliseconds());
  }
  return 0;
}

} // namespace model
