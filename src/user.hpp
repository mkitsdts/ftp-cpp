#pragma once
#include <string_view>

namespace ftp {

class User {
public:
  static bool handle_user(std::string_view username);
  static bool handle_pass(std::string_view username, std::string_view password);

private:
  User() = default;
  ~User() = default;
  User(const User &) = delete;
  User(User &&) = delete;
  User &operator=(const User &) = delete;
  User &operator=(User &&) = delete;
};

}; // namespace ftp