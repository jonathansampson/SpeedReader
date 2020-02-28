#include "../include/speedreader.hpp"

#include <iostream>

#include "../include/speedreader_ffi.hpp"

#define UNUSED (void)

namespace speedreader {

SpeedReader::SpeedReader() : raw(speedreader_new()) {}
SpeedReader::SpeedReader(const char* whitelist_serialized,
                         size_t whitelist_size)
    : raw(speedreader_with_whitelist(whitelist_serialized, whitelist_size)) {}

SpeedReader::~SpeedReader() {
  speedreader_free(raw);
}

bool SpeedReader::ReadableURL(const std::string& url) {
  return speedreader_url_readable(raw, url.c_str(), url.length());
}

RewriterType SpeedReader::RewriterTypeForURL(const std::string& url) {
  return speedreader_find_type(raw, url.c_str(), url.length());
}

std::unique_ptr<Rewriter> SpeedReader::RewriterNew(const std::string& url) {
  return std::make_unique<Rewriter>(raw, url, RewriterType::RewriterUnknown);
}

std::unique_ptr<Rewriter> SpeedReader::RewriterNew(const std::string& url,
                                                   RewriterType rewriter_type) {
  return std::make_unique<Rewriter>(raw, url, rewriter_type);
}

std::unique_ptr<Rewriter> SpeedReader::RewriterNew(
    const std::string& url,
    RewriterType rewriter_type,
    std::function<void(const char*, size_t)> callback) {
  return std::make_unique<Rewriter>(raw, url, rewriter_type, callback);
}

Rewriter::Rewriter(C_SpeedReader* speedreader,
                   const std::string& url,
                   RewriterType rewriter_type)
    : output_(""),
      ended_(false),
      raw(speedreader_rewriter_new(
          speedreader,
          url.c_str(),
          url.length(),
          [](const char* chunk, size_t chunk_len, void* user_data) {
            std::string* out = static_cast<std::string*>(user_data);
            out->append(chunk, chunk_len);
          },
          &output_,
          rewriter_type)) {}

Rewriter::Rewriter(C_SpeedReader* speedreader,
                   const std::string& url,
                   RewriterType rewriter_type,
                   std::function<void(const char*, size_t)> callback)
    : output_(""),
      ended_(false),
      raw(speedreader_rewriter_new(
          speedreader,
          url.c_str(),
          url.length(),
          [](const char* chunk, size_t chunk_len, void* user_data) {
            auto* callback =
                static_cast<std::function<void(const char*, size_t)>*>(
                    user_data);
            (*callback)(chunk, chunk_len);
          },
          &callback,
          rewriter_type)) {}

Rewriter::~Rewriter() {
  if (!ended_) {
    speedreader_rewriter_free(raw);
  }
}

int Rewriter::Write(const char* chunk, size_t chunk_len) {
  if (!ended_) {
    return speedreader_rewriter_write(raw, chunk, chunk_len);
  } else {
    return -1;
  }
}

int Rewriter::End() {
  if (!ended_) {
    int ret = speedreader_rewriter_end(raw);
    ended_ = true;
    return ret;
  } else {
    return -1;
  }
}

const std::string* Rewriter::GetOutput() {
  return &output_;
}

}  // namespace speedreader
