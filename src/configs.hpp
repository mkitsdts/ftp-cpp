#pragma once
#include <string>
#include <unordered_map>
namespace ftp {

const std::size_t BUFFER_SIZE = 1024;  // 缓冲区大小
const std::size_t MAX_CONNECTIONS = 5; // 最大连接数

const int PORT = 21; // FTP 默认端口

const std::string ROOT_PATH = "./files";

const std::unordered_map<std::string, std::string> USER_INFO = {
    {"root", "root"},
    {"user", "user"},
};

} // namespace ftp