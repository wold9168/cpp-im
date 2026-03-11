#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace req {

enum class Method { GET, POST, UNKNOWN };

inline std::string_view method_to_string(Method method) {
  switch (method) {
  case Method::GET:
    return "GET";
  case Method::POST:
    return "POST";
  default:
    return "UNKNOWN";
  }
}

struct HttpRequest {
  Method method = Method::UNKNOWN;
  std::string path;
  std::string query;
  std::string version;
  std::unordered_map<std::string, std::string> headers;
  std::string body;

  std::string get_header(const std::string &key) const {
    auto it = headers.find(key);
    if (it != headers.end()) {
      return it->second;
    }
    return "";
  }
};

enum class ParseState { RequestLine, Headers, Body, Complete, Error };

class Parser {
public:
  Parser() = default;

  /// Parse HTTP request from buffer
  /// Returns number of bytes consumed, or 0 if incomplete
  size_t parse(std::string_view data, HttpRequest &request);

  /// Get last error message
  const std::string &get_error() const { return error_message_; }

  /// Check if parser is in error state
  bool has_error() const { return state_ == ParseState::Error; }

  /// Reset parser state
  void reset();

private:
  ParseState state_ = ParseState::RequestLine;
  std::string error_message_;
  size_t bytes_received_ = 0;
  size_t content_length_ = 0;
  size_t body_bytes_read_ = 0;

  bool parse_request_line(std::string_view &data, HttpRequest &request);
  bool parse_headers(std::string_view &data, HttpRequest &request);
  bool parse_body(std::string_view &data, HttpRequest &request);

  static std::string trim(std::string_view str);
  static std::string to_lower(std::string_view str);
};

} // namespace req
