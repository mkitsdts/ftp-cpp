#include "server.hpp"
#include "configs.hpp"
#include "parser.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <filesystem>
#include <fstream>
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
    clients[ip].curr_path = ROOT_PATH; // 设置当前路径为根目录
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
    case Command::GET:
      handle_get(ip, command);
      break;
    case Command::RETR:
      handle_get(ip, command);
      break;
    case Command::QUIT:
      handle_quit(ip);
      break;
    case Command::PWD:
      handle_pwd(ip, command);
      break;
    case Command::LCD:
      handle_lcd(ip, command);
      break;
    case Command::SYST:
      handle_syst(ip);
      break;
    case Command::EPSV:
      handle_epsv(ip);
      break;
    case Command::PASV:
      handle_pasv(ip);
      break;
    case Command::TYPE: {
      std::string response = "200 Type set to I\r\n";
      send(clients[std::string(ip)].client_fd, response.c_str(),
           response.size(), 0);
      break;
    }
    case Command::ERROR: {
      std::cerr << "Invalid command" << std::endl;
      std::string error_msg = "500 Unknown command\r\n";
      send(client_fd, error_msg.c_str(), error_msg.size(), 0);
      break;
    }
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
    std::string response = "331 User name okay, need password\r\n";
    send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
         0);
    return COMMON;
  }
  return SERVER_INNER_ERROR;
}

int FtpServer::handle_pass(std::string_view ip, std::string_view password) {
  auto username = users[std::string(ip)];
  if (USER_INFO.size() == 0) {
    std::cerr << "No user information available" << std::endl;
    std::string mess = "530 Login incorrect\r\n";
    send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
    return SERVER_INNER_ERROR;
  }
  for (const auto &user : USER_INFO) {
    if (user.first == username && user.second == password) {
      {
        std::lock_guard<std::mutex> lock(login_status_mutex);
        login_status[std::string(ip)] = true;
        std::string mess = "230 User logged in, proceed\r\n";
        send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
      }
      return COMMON;
    }
  }
  std::string mess = "530 Login incorrect\r\n";
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
    std::string directory;
    if (path == "") {
      directory = clients[std::string(ip)].curr_path; // 默认当前目录
    } else {
      directory = clients[std::string(ip)].curr_path + '/' + std::string(path);
    }

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

int FtpServer::handle_quit(std::string_view ip) {
  // 退出登录
  std::cout << "User " << users[std::string(ip)] << " logged out from " << ip
            << std::endl;
  std::string mess = "221 Goodbye\r\n";
  send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
  close(clients[std::string(ip)].client_fd);
  {
    std::lock_guard<std::mutex> lock(users_mutex);
    users.erase(std::string(ip));
  }
  {
    std::lock_guard<std::mutex> lock(login_status_mutex);
    login_status.erase(std::string(ip));
  }
  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients[std::string(ip)].client_fd = -1;
    clients[std::string(ip)].data_fd = -1;
    clients[std::string(ip)].curr_path = "";
    clients.erase(std::string(ip));
  }
  return COMMON;
}

int FtpServer::handle_get(std::string_view ip, std::string_view path) {
  // 检查登陆状态
  if (login_status[std::string(ip)] == false) {
    std::string mess = "530 Not logged in\r\n";
    send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
    return SERVER_INNER_ERROR;
  }

  // 检查数据连接是否可用
  if (clients[std::string(ip)].data_fd == -1) {
    std::string mess = "425 Use PASV first\r\n";
    send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
    return SERVER_INNER_ERROR;
  }

  try {
    std::string file_path = ROOT_PATH + "/" + path.data();

    std::cout << "Requesting file: " << path << std::endl;
    std::cout << "Full path: " << file_path << std::endl;

    // 检查文件是否存在
    if (!std::filesystem::exists(file_path)) {
      std::string error_msg = "550 File not found\r\n";
      send(clients[std::string(ip)].client_fd, error_msg.c_str(),
           error_msg.size(), 0);
      return SERVER_INNER_ERROR;
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
      std::string error_msg = "550 Failed to open file\r\n";
      send(clients[std::string(ip)].client_fd, error_msg.c_str(),
           error_msg.size(), 0);
      return SERVER_INNER_ERROR;
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 发送 150 响应到控制连接
    std::string response = std::format(
        "150 Opening BINARY mode data connection for {} bytes\r\n", file_size);
    send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
         0);

    // 等待客户端连接数据端口
    int listen_fd = clients[std::string(ip)].data_fd;
    sockaddr_in client_data_addr;
    socklen_t addr_len = sizeof(client_data_addr);

    std::cout << "Waiting for data connection..." << std::endl;
    int data_fd =
        accept(listen_fd, (struct sockaddr *)&client_data_addr, &addr_len);
    if (data_fd < 0) {
      std::cerr << "Accept data connection failed" << std::endl;
      std::string error_msg = "425 Cannot open data connection\r\n";
      send(clients[std::string(ip)].client_fd, error_msg.c_str(),
           error_msg.size(), 0);
      file.close();
      close(listen_fd);
      clients[std::string(ip)].data_fd = -1;
      return SERVER_INNER_ERROR;
    }

    std::cout << "Data connection established" << std::endl;

    // 通过数据连接发送文件内容
    char buffer[BUFFER_SIZE];
    size_t total_sent = 0;

    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
      size_t bytes_read = file.gcount();
      ssize_t bytes_sent = send(data_fd, buffer, bytes_read, 0);

      if (bytes_sent < 0) {
        std::cerr << "Failed to send file data" << std::endl;
        break;
      }
      total_sent += bytes_sent;
    }

    file.close();
    close(data_fd);
    close(listen_fd);
    clients[std::string(ip)].data_fd = -1;

    // 发送传输完成消息到控制连接
    response = "226 Transfer complete\r\n";
    send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
         0);

    std::cout << "File " << path << " sent to " << ip << " (" << total_sent
              << " bytes)" << std::endl;

  } catch (const std::exception &ex) {
    std::cerr << "Error in handle_get: " << ex.what() << std::endl;
    std::string error_msg = "550 Transfer failed\r\n";
    send(clients[std::string(ip)].client_fd, error_msg.c_str(),
         error_msg.size(), 0);
    return SERVER_INNER_ERROR;
  }

  return COMMON;
}

int FtpServer::handle_pwd(std::string_view ip, std::string_view path) {
  // 检查登陆状态
  if (login_status[std::string(ip)] == false) {
    std::cerr << "User not logged in" << std::endl;
    std::string mess = "Please login first\r\n";
    send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
    return SERVER_INNER_ERROR;
  }
  std::string response = "257 \"" + clients[std::string(ip)].curr_path +
                         "\" is the current directory\r\n";
  send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
       0);
  return COMMON;
}

int FtpServer::handle_lcd(std::string_view ip, std::string_view path) {
  // 检查登陆状态
  if (login_status[std::string(ip)] == false) {
    std::cerr << "User not logged in" << std::endl;
    std::string mess = "Please login first\r\n";
    send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
    return SERVER_INNER_ERROR;
  }
  // 切换目录
  clients[std::string(ip)].curr_path =
      clients[std::string(ip)].curr_path + '/' + path.data();
  std::string response =
      "250 Directory changed to " + std::string(path) + "\r\n";
  send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
       0);
  return COMMON;
}

int FtpServer::handle_syst(std::string_view ip) {
  // 检查登陆状态
  if (login_status[std::string(ip)] == false) {
    std::cerr << "User not logged in" << std::endl;
    std::string mess = "Please login first\r\n";
    send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
    return SERVER_INNER_ERROR;
  }
  std::string response = "215 UNIX Type: L8\r\n";
  send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
       0);
  return COMMON;
}

int FtpServer::handle_pasv(std::string_view ip) {
  // 检查登陆状态
  if (login_status[std::string(ip)] == false) {
    std::cerr << "User not logged in" << std::endl;
    std::string mess = "Please login first\r\n";
    send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
    return SERVER_INNER_ERROR;
  }
  // 分配一个新的套接字用于数据传输
  int data_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (data_fd < 0) {
    std::cerr << "Failed to create data socket" << std::endl;
    return SERVER_INNER_ERROR;
  }
  // 绑定套接字到地址
  sockaddr_in data_addr;
  data_addr.sin_family = AF_INET;
  data_addr.sin_addr.s_addr = INADDR_ANY;
  data_addr.sin_port = 0; // 让系统自动分配端口
  if (bind(data_fd, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
    std::cerr << "Bind failed" << std::endl;
    close(data_fd);
    return SERVER_INNER_ERROR;
  }
  // 更新客户端信息
  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients[std::string(ip)].data_fd = data_fd;
  }

  // 开始监听数据连接
  if (listen(data_fd, 1) < 0) {
    std::cerr << "Listen failed" << std::endl;
    close(data_fd);
    std::string error_msg = "425 Cannot open data connection\r\n";
    send(clients[std::string(ip)].client_fd, error_msg.c_str(),
         error_msg.size(), 0);
    return SERVER_INNER_ERROR;
  }

  // 获取分配的端口号
  socklen_t addr_len = sizeof(data_addr);
  if (getsockname(data_fd, (struct sockaddr *)&data_addr, &addr_len) < 0) {
    std::cerr << "Get socket name failed: " << strerror(errno) << std::endl;
    close(data_fd);
    std::string error_msg = "425 Cannot open data connection\r\n";
    send(clients[std::string(ip)].client_fd, error_msg.c_str(),
         error_msg.size(), 0);
    return SERVER_INNER_ERROR;
  }
  int port = ntohs(data_addr.sin_port);

  std::string response =
      std::format("227 Entering Passive Mode (127,0,0,1,{},{})\r\n", port / 256,
                  port % 256);
  std::cout << "Passive mode: " << response << std::endl;
  // 发送响应
  send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
       0);
  std::cout << "Data connection established" << std::endl;
  return COMMON;
}

int FtpServer::handle_epsv(std::string_view ip) {
  // 检查登陆状态
  //   if (login_status[std::string(ip)] == false) {
  //     std::cerr << "User not logged in" << std::endl;
  //     std::string mess = "Please login first\r\n";
  //     send(clients[std::string(ip)].client_fd, mess.c_str(), mess.size(), 0);
  //     return SERVER_INNER_ERROR;
  //   }
  //   std::string response = "229 Entering Extended Passive Mode (|||0|)\r\n";
  //   send(clients[std::string(ip)].client_fd, response.c_str(),
  //   response.size(),
  //        0);
  //   return COMMON;
  std::string response = "500 Not Provided\r\n";
  send(clients[std::string(ip)].client_fd, response.c_str(), response.size(),
       0);
  return COMMON;
}