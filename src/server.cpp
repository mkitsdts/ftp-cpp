#include "server.hpp"
#include "configs.hpp"
#include "parser.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace ftp;

inline std::unordered_map<std::string, std::string> FtpServer::users;
// 保护 users 的互斥锁
inline std::mutex FtpServer::users_mutex;
// IP 与 客户端信息的映射
inline std::unordered_map<std::string, Client> FtpServer::clients;
// 保护 clients 的互斥锁
inline std::mutex FtpServer::clients_mutex;
// 登陆状态
inline std::unordered_map<std::string, bool> FtpServer::login_status;
// 保护 login_status 的互斥锁
inline std::mutex FtpServer::login_status_mutex;

void FtpServer::start() {
  // 创建一个服务器套接字
  // AF_INET 表示 IPv4，SOCK_STREAM 表示 TCP
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == CREATE_SOCKET_ERROR) {
    std::cerr << "Failed to create socket" << std::endl;
    exit(CREATE_SOCKET_ERROR);
  }
  // 储存服务器地址信息
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT); // FTP default port

  // 绑定套接字到地址
  if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "Bind failed" << std::endl;
    close(server_fd);
    exit(BIND_SOCKET_ERROR);
  }
  // 开始监听连接
  if (listen(server_fd, MAX_CONNECTIONS) < 0) {
    std::cerr << "Listen failed" << std::endl;
    close(server_fd);
    exit(BIND_SOCKET_ERROR);
  }
  std::cout << "FTP server started on port 21" << std::endl;
  while (true) {
    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0) {
      std::cerr << "Accept failed" << std::endl;
      continue;
    }
    auto tmp_thread = std::thread(&FtpServer::handle_client,
                                  std::ref(client_addr), client_fd);
    tmp_thread.detach();
  }
}

void FtpServer::stop() {
  // Implementation of stopping the FTP server
}

int FtpServer::handle_client(sockaddr_in &addr, int client_fd) {
  char buffer[BUFFER_SIZE];
  std::string ip = inet_ntoa(addr.sin_addr);
  std::cout << "Client connected: " << ip << std::endl;
  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients[ip].address = addr;
    clients[ip].client_fd = client_fd;
    clients_mutex.unlock();
  }
  std::string welcome_msg = "220 Welcome to the FTP server\r\n";
  send(client_fd, welcome_msg.c_str(), welcome_msg.size(), 0);
  while (true) {
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
      std::cerr << "Client disconnected: " << ip << std::endl;
      close(client_fd);
      break;
    }
    std::string command(buffer);
    std::cout << "Received command from " << ip << ": " << command << std::endl;
    Command cmd = Parser::parse(command);
    switch (cmd) {
    case Command::USER:
      handle_user(ip, command);
      break;
    case Command::PASS:
      handle_pass(ip, command);
      break;
    case Command::LIST:
      handle_list(ip, command);
      break;
    case Command::DIR:
      // wait for achieve
      break;
    case Command::GET:
      // wait for achieve
      break;
    default:
      std::cerr << "Invalid command" << std::endl;
      break;
    }
  }
  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(ip);
  }
  return COMMON;
}

int FtpServer::handle_user(std::string_view ip, std::string_view username) {
  {
    std::lock_guard<std::mutex> lock(users_mutex);
    users[std::string(ip)] = username;
    std::cout << "User " << username << " logged in from " << ip << std::endl;
    return COMMON;
  }
  return SERVER_INNER_ERROR;
}

int FtpServer::handle_pass(std::string_view ip, std::string_view password) {
  auto username = users[std::string(ip)];
  if (USER_INFO.size() == 0) {
    std::cerr << "No user information available" << std::endl;
    std::string mess = "wrong password\r\n";
    send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
    return SERVER_INNER_ERROR;
  }
  for (const auto &user : USER_INFO) {
    if (user.first == username && user.second == password) {
      {
        std::lock_guard<std::mutex> lock(login_status_mutex);
        login_status[std::string(ip)] = true;
        std::string mess = "logged in successfully\r\n";
        send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
      }
      return COMMON;
    }
  }
  std::string mess = "Invalid password for user\r\n";
  send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
  return SERVER_INNER_ERROR;
}

int FtpServer::handle_list(std::string_view ip, std::string_view path) {
  // 检查登陆状态
  if (login_status[std::string(ip)] == false) {
    std::cerr << "User not logged in" << std::endl;
    std::string mess = "Please login first\r\n";
    send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
    return SERVER_INNER_ERROR;
  }
  try {
    std::string directory = ROOT_PATH + std::string(path); // 默认当前目录

    std::stringstream file_list;
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
      const auto &path = entry.path();
      const auto &status = entry.status();

      // 获取文件信息
      std::string permissions;
      if (std::filesystem::is_directory(status)) {
        permissions = "drwxr-xr-x";
      } else {
        permissions = "-rw-r--r--";
      }

      // 获取文件大小
      uintmax_t file_size = 0;
      if (std::filesystem::is_regular_file(status)) {
        file_size = std::filesystem::file_size(path);
      }

      // 获取修改时间
      auto ftime = std::filesystem::last_write_time(path);
      auto sctp =
          std::chrono::time_point_cast<std::chrono::system_clock::duration>(
              ftime - std::filesystem::file_time_type::clock::now() +
              std::chrono::system_clock::now());
      std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

      // 格式化文件信息 (类似 ls -l 格式)
      char time_buffer[100];
      std::strftime(time_buffer, sizeof(time_buffer), "%b %d %H:%M",
                    std::localtime(&cftime));

      file_list << permissions << " 1 user group " << std::setw(10) << file_size
                << " " << time_buffer << " " << path.filename().string()
                << "\r\n";
    }

    // 发送文件列表
    std::string response = "150 Here comes the directory listing\r\n";
    send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
         0);

    std::string list_data = file_list.str();
    send(clients[std::string(ip)].client_fd, list_data.c_str(),
         list_data.size(), 0);

    response = "226 Directory send OK\r\n";
    send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
         0);

  } catch (const std::filesystem::filesystem_error &ex) {
    std::cerr << "Filesystem error: " << ex.what() << std::endl;
    std::string error_msg = "550 Failed to open directory\r\n";
    send(clients[std::string(ip)].client_fd, error_msg.c_str(),
         error_msg.size(), 0);
    return SERVER_INNER_ERROR;
  }
  return COMMON;
}
