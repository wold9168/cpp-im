#include "req/parser.hpp"
#include <algorithm>
#include <cctype>

namespace req {

void Parser::reset() {
  state_ = ParseState::RequestLine;
  error_message_.clear();
  bytes_received_ = 0;
  content_length_ = 0;
  body_bytes_read_ = 0;
}

std::string Parser::trim(std::string_view str) {
  size_t start = 0;
  size_t end = str.size();

  while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
    ++start;
  }
  while (end > start &&
         std::isspace(static_cast<unsigned char>(str[end - 1]))) {
    --end;
  }

  return std::string(str.substr(start, end - start));
}

std::string Parser::to_lower(std::string_view str) {
  std::string result(str);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

size_t Parser::parse(std::string_view data, HttpRequest &request) {
  if (data.empty()) {
    return 0;
  }

  size_t total_consumed = 0;

  while (!data.empty() && state_ != ParseState::Complete &&
         state_ != ParseState::Error) {
    switch (state_) {
    case ParseState::RequestLine:
      if (!parse_request_line(data, request)) {
        return 0; // Incomplete
      }
      total_consumed += bytes_received_;
      bytes_received_ = 0;
      break;

    case ParseState::Headers:
      if (!parse_headers(data, request)) {
        return 0; // Incomplete
      }
      total_consumed += bytes_received_;
      bytes_received_ = 0;
      break;

    case ParseState::Body:
      if (!parse_body(data, request)) {
        return 0; // Incomplete
      }
      total_consumed += bytes_received_;
      bytes_received_ = 0;
      break;

    default:
      break;
    }
  }

  if (state_ == ParseState::Error) {
    return 0;
  }

  return total_consumed;
}

bool Parser::parse_request_line(std::string_view &data, HttpRequest &request) {
  // Find end of line
  size_t pos = data.find("\r\n");
  if (pos == std::string_view::npos) {
    // Check for just \n (less common but some clients send this)
    pos = data.find('\n');
    if (pos == std::string_view::npos) {
      bytes_received_ = data.size();
      return false; // Incomplete
    }
  }

  std::string_view line = data.substr(0, pos);
  size_t consumed = pos + 1;
  if (pos + 1 < data.size() && data[pos + 1] == '\r') {
    consumed = pos + 2;
  }

  // Parse: METHOD SP PATH SP VERSION
  size_t first_space = line.find(' ');
  if (first_space == std::string_view::npos) {
    error_message_ = "Invalid request line: missing method";
    state_ = ParseState::Error;
    data = data.substr(consumed);
    return true;
  }

  std::string_view method_str = line.substr(0, first_space);

  size_t second_space = line.find(' ', first_space + 1);
  if (second_space == std::string_view::npos) {
    error_message_ = "Invalid request line: missing path";
    state_ = ParseState::Error;
    data = data.substr(consumed);
    return true;
  }

  std::string_view path_str =
      line.substr(first_space + 1, second_space - first_space - 1);
  std::string_view version_str = line.substr(second_space + 1);

  // Parse method
  if (method_str == "GET") {
    request.method = Method::GET;
  } else if (method_str == "POST") {
    request.method = Method::POST;
  } else {
    error_message_ = "Unsupported method: " + std::string(method_str);
    state_ = ParseState::Error;
    data = data.substr(consumed);
    return true;
  }

  // Parse path and query string
  size_t query_pos = path_str.find('?');
  if (query_pos != std::string_view::npos) {
    request.path = std::string(path_str.substr(0, query_pos));
    request.query = std::string(path_str.substr(query_pos + 1));
  } else {
    request.path = std::string(path_str);
  }

  request.version = std::string(version_str);

  // Validate HTTP version
  if (request.version != "HTTP/1.1") {
    error_message_ = "Unsupported HTTP version: " + request.version;
    state_ = ParseState::Error;
    data = data.substr(consumed);
    return true;
  }

  data = data.substr(consumed);
  state_ = ParseState::Headers;
  return true;
}

bool Parser::parse_headers(std::string_view &data, HttpRequest &request) {
  while (!data.empty()) {
    // Find end of line
    size_t pos = data.find("\r\n");
    if (pos == std::string_view::npos) {
      pos = data.find('\n');
      if (pos == std::string_view::npos) {
        bytes_received_ = data.size();
        return false; // Incomplete
      }
    }

    std::string_view line = data.substr(0, pos);
    size_t consumed = pos + 1;
    if (pos + 1 < data.size() && data[pos + 1] == '\r') {
      consumed = pos + 2;
    }

    // Empty line marks end of headers
    if (line.empty()) {
      data = data.substr(consumed);

      // Determine if we need to read a body
      auto it = request.headers.find("content-length");
      if (it != request.headers.end()) {
        try {
          content_length_ = std::stoul(it->second);
        } catch (...) {
          error_message_ = "Invalid Content-Length header";
          state_ = ParseState::Error;
          return true;
        }

        if (content_length_ > 0) {
          state_ = ParseState::Body;
        } else {
          state_ = ParseState::Complete;
        }
      } else {
        // No Content-Length header, request is complete
        // (for GET requests or POST without body)
        state_ = ParseState::Complete;
      }

      return true;
    }

    // Parse header: Name: Value
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string_view::npos) {
      error_message_ = "Invalid header line: " + std::string(line);
      state_ = ParseState::Error;
      data = data.substr(consumed);
      return true;
    }

    std::string name = trim(line.substr(0, colon_pos));
    std::string value = trim(line.substr(colon_pos + 1));

    // Store header name in lowercase for case-insensitive lookup
    request.headers[to_lower(name)] = value;

    data = data.substr(consumed);
  }

  bytes_received_ = 0;
  return true;
}

bool Parser::parse_body(std::string_view &data, HttpRequest &request) {
  size_t remaining = content_length_ - body_bytes_read_;
  size_t to_read = std::min(data.size(), remaining);

  request.body.append(data.substr(0, to_read));
  body_bytes_read_ += to_read;

  if (body_bytes_read_ >= content_length_) {
    state_ = ParseState::Complete;
    data = data.substr(to_read);
    return true;
  }

  data = data.substr(to_read);
  bytes_received_ = 0;
  return false; // Still need more data
}

} // namespace req
