#pragma once
#include <netinet/in.h>

namespace ftp {

const int COMMON = 0; // 普通返回值
const int CREATE_SOCKET_ERROR = -1;
const int BIND_SOCKET_ERROR = -2;
const int SERVER_INNER_ERROR = -3;

enum class Command {
  USER,
  PASS,
  LIST,
  DIR,
  GET,
  ERROR,
};

struct Client {
  sockaddr_in address; // 客户端地址
  int client_fd;       // 客户端文件描述符
};
} // namespace ftp