#include "cppkit/arg_parser.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  cppkit::ArgParser parser;
  parser.addOption("--port", "Server port", "8080");
  parser.addOption("--host", "Server host", "localhost");
  parser.addFlag("-h", "Show help");

  parser.parse(argc, argv);

  if (parser.has("-h")) {
    std::cout << parser.help(argv[0]);
    return 0;
  }

  std::cout << "Host: " << parser.get("--host") << "\n";
  std::cout << "Port: " << parser.get("--port") << "\n";
  return 0;
}