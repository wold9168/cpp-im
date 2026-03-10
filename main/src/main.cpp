#include "main/main.hpp"
#include <argparse/argparse.hpp>
#include <cstdlib>

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

  program.add_argument("-h", "--help")
      .help("show help info")
      .default_value(false)
      .implicit_value(true)
      .nargs(0);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  argparse::ArgumentParser program("main", "1.0",
                                   argparse::default_arguments::none);
  argparse_initialize(program, argc, argv);

  if (program["--help"] == true) {
    std::cout << program;
    exit(EXIT_SUCCESS);
  }
  return 0;
}
