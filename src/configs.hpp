#pragma once
#include <string>
#include <unordered_map>
namespace ftp {

constexpr int BUFFER_SIZE = 1024;  // 缓冲区大小
constexpr int MAX_CONNECTIONS = 5; // 最大连接数

constexpr int MAX_PORT = 65540; // 最大端口号
constexpr int MIN_PORT = 1024;  // 最小端口号

constexpr int PORT = 21; // FTP 默认端口

const std::string ROOT_PATH = "./files";

const std::unordered_map<std::string, std::string> USER_INFO = {
    {"root", "root"},
    {"user", "user"},
};

} // namespace ftp