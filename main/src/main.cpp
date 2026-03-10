#include "main/main.hpp"
#include "main/version.h"
#include <argparse/argparse.hpp>
#include <cstdlib>
#include <spdlog/spdlog.h>

bool verbose = false;

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

  program.add_argument("-p", "--port")
      .help("specify the port (default=8080)")
      .default_value(8080)
      .nargs(1)
      .scan<'d', int>();

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    exit(EXIT_FAILURE);
  }
}

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
  return 0;
}
