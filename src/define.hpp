#pragma once
#include <netinet/in.h>
#include <string>
namespace ftp {

constexpr int COMMON = 0; // 普通返回值
constexpr int CREATE_SOCKET_ERROR = -1;
constexpr int BIND_SOCKET_ERROR = -2;
constexpr int SERVER_INNER_ERROR = -3;

enum class Command {
  USER,
  PASS,
  LIST,
  GET,
  LCD,
  QUIT,
  RETR,
  PWD,
  SYST,
  EPSV,
  PASV,
  TYPE,
  ERROR,
};

struct Client {
  sockaddr_in address;   // 客户端地址
  int client_fd;         // 通信文件描述符
  int data_fd;           // 数据传输文件描述符
  std::string curr_path; // 当前路径
};
} // namespace ftp