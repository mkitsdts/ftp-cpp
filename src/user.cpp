#include "user.hpp"
#include "configs.hpp"
using namespace ftp;

// 判断用户是否存在
bool User::handle_user(std::string_view username) {
  for (const auto &user : USER_INFO) {
    if (user.first == username) {
      return true;
    }
  }
  return false;
}

// 校验密码
bool User::handle_pass(std::string_view username, std::string_view password) {
  auto it = USER_INFO.find(std::string(username));
  if (it != USER_INFO.end() && it->second == password) {
    return true;
  }
  return false;
}