#pragma once
#include "define.hpp"
#include <string>

namespace ftp {

class Parser {
public:
  static Command parse(std::string &command);
  static std::pair<std::string, int> parse_path(std::string_view ip);

private:
  Parser() = default;
  ~Parser() = default;
  Parser(const Parser &) = delete;
  Parser(Parser &&) = delete;
  Parser &operator=(const Parser &) = delete;
  Parser &operator=(Parser &&) = delete;
  static void trim_ftp_command(std::string &str);
  static void Upper(std::string &str);
  static bool is_valid_command(const std::string &command);
};

} // namespace ftp