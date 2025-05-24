#include "user.hpp"
#include "configs.hpp"
using namespace ftp;

// �ж��û��Ƿ����
bool User::handle_user(std::string_view username) {
  for (const auto &user : USER_INFO) {
    if (user.first == username) {
      return true;
    }
  }
  return false;
}

// У������
bool User::handle_pass(std::string_view username, std::string_view password) {
  auto it = USER_INFO.find(std::string(username));
  if (it != USER_INFO.end() && it->second == password) {
    return true;
  }
  return false;
}