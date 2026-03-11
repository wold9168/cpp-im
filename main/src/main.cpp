#include "main/main.hpp"
#include "main/version.h"
#include <argparse/argparse.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <req/parser.hpp>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

bool verbose = false;

#define CHECK_CALL(func, ...) check_error(#func, func(__VA_ARGS__))
int check_error(const char *msg, int res) {
  if (res == -1) {
    spdlog::error("{}: {}", msg, strerror(res));
    throw;
  }
  return res;
}

size_t check_error(const char *msg, ssize_t res) {
  if (res == -1) {
    spdlog::error("{}: {}", msg, strerror(res));
    throw;
  }
  return res;
}

void argparse_initialize(argparse::ArgumentParser &program, const int &argc,
                         const pt2pt2char &argv) {
  program.add_description("HTTP server.");
  program.add_epilog("Written by wold9168.");
  program.add_argument("-v", "--verbose")
      .help("enable verbose mode")
      .default_value(false)
      .implicit_value(true)
      .store_into(verbose)
      .nargs(0);

  program.add_argument("-V", "--version")
      .help("show version")
      .default_value(false)
      .implicit_value(true)
      .nargs(0);

  program.add_argument("-h", "--help")
      .help("show help info")
      .default_value(false)
      .implicit_value(true)
      .nargs(0);

  program.add_argument("-l", "--listen")
      .help("specify the listen addr (default=127.0.0.1)")
      .default_value("127.0.0.1")
      .nargs(1);

  program.add_argument("-p", "--port")
      .help("specify the port (default=8080)")
      .default_value(8080)
      .nargs(1);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    exit(EXIT_FAILURE);
  }
}

struct SockaddrFat {
  struct sockaddr *m_addr;
  socklen_t m_addrlen;
};

class SockAddrStorage {
public:
  union {
    struct sockaddr m_addr;
    struct sockaddr_storage m_addr_storage;
  };
  socklen_t m_addrlen = sizeof(struct sockaddr_storage);
  operator SockaddrFat() { return {&m_addr, m_addrlen}; }
};

class AddrResolvedEntry {
private:
  struct addrinfo *m_cur = nullptr;

public:
  AddrResolvedEntry(struct addrinfo *cur) : m_cur(cur) {};

  SockaddrFat get_addr() const { return {m_cur->ai_addr, m_cur->ai_addrlen}; }

  int create_socket() const {
    return CHECK_CALL(socket, m_cur->ai_family, m_cur->ai_socktype,
                      m_cur->ai_protocol);
  }
  bool next_entry() {
    m_cur = m_cur->ai_next;
    return m_cur != nullptr;
  }
  int create_socket_and_bind() const {
    int sockfd = create_socket();
    SockaddrFat addrfat = get_addr();
    CHECK_CALL(bind, sockfd, addrfat.m_addr, addrfat.m_addrlen);
    return sockfd;
  }
};

class AddrResolver {
private:
  struct addrinfo *m_head = nullptr;

public:
  void resolve(const std::string &name, const std::string &srv) {
    int gaierr = getaddrinfo(name.c_str(), srv.c_str(), NULL, &m_head);
    if (gaierr != 0) {
      spdlog::error("getaddrinfo: {}", gai_strerror(gaierr));
      throw;
    }
  }
  AddrResolvedEntry get_first_entry() { return {m_head}; }
  AddrResolver() = default;
  AddrResolver(AddrResolver &&that) : m_head(that.m_head) {
    that.m_head = nullptr;
  }
  ~AddrResolver() {
    if (m_head) {
      freeaddrinfo(m_head);
    }
  }
};

int main(int argc, char **argv) {
  argparse::ArgumentParser program("main", APP_VERSION,
                                   argparse::default_arguments::none);
  argparse_initialize(program, argc, argv);

  if (program["--help"] == true) {
    std::cout << program;
    exit(EXIT_SUCCESS);
  } else if (program["--version"] == true) {
    std::cout << APP_VERSION;
    exit(EXIT_SUCCESS);
  }

  if (program["--verbose"] == true) {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Verbose output enabled.");
  }
  auto addr = program.get<std::string>("--listen");
  auto port = program.get<std::string>("--port");
  AddrResolver ar;
  ar.resolve(addr, port);
  auto entry = ar.get_first_entry();
  auto listen_sockfd = entry.create_socket_and_bind();
  CHECK_CALL(listen, listen_sockfd, SOMAXCONN);
  while (true) {
    SockAddrStorage client_addr;
    int peerfd = CHECK_CALL(accept, listen_sockfd, &client_addr.m_addr,
                            &client_addr.m_addrlen);
    char buf[4096];
    size_t n = CHECK_CALL(read, peerfd, buf, sizeof(buf));

    req::Parser parser;
    req::HttpRequest request;
    size_t consumed = parser.parse({buf, n}, request);

    if (parser.has_error()) {
      spdlog::error("Parse error: {}", parser.get_error());
    } else if (consumed > 0) {
      spdlog::info("Received {} request for {}",
                   req::method_to_string(request.method), request.path);
      if (!request.query.empty()) {
        spdlog::debug("Query string: {}", request.query);
      }
      for (const auto &[key, value] : request.headers) {
        spdlog::debug("Header: {} = {}", key, value);
      }
      if (!request.body.empty()) {
        spdlog::debug("Body: {}", request.body);
      }
    }

    close(peerfd);
  }
}
