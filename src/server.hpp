#pragma once
#include "define.hpp"
#include <mutex>
#include <string_view>
#include <unordered_map>

namespace ftp {

class FtpServer {
public:
  static void start();
  static void stop();

private:
  FtpServer() = default;
  ~FtpServer() = default;
  FtpServer(const FtpServer &) = delete;
  FtpServer(FtpServer &&) = delete;
  FtpServer &operator=(const FtpServer &) = delete;
  FtpServer &operator=(FtpServer &&) = delete;

  static int handle_client(sockaddr_in &address,
                           int client_fd); // 分配线程处理客户端请求
  static int handle_user(std::string_view ip,
                         std::string_view username); // 记录用户登录
  static int handle_pass(std::string_view ip,
                         std::string_view password); // 校验用户密码
  static int handle_list(std::string_view ip,
                         std::string_view path); // 列出文件
  static int handle_get(std::string_view ip,
                        std::string_view path); // 下载文件
  static int handle_pwd(std::string_view ip,
                        std::string_view path); // 显示当前目录
  static int handle_lcd(std::string_view ip,
                        std::string_view path); // 切换目录
  static int handle_port(std::string_view ip,
                         std::string_view path); // 主动模式
  static int handle_syst(std::string_view ip);   // 显示系统信息
  static int handle_pasv(std::string_view ip);   // 主动模式
  static int handle_epsv(std::string_view ip);   // 被动模式
  static int handle_quit(std::string_view ip);   // 退出登录
  static int handle_type(std::string_view ip);   // 设置传输类型
  static int handle_error(std::string_view ip);  // 处理错误
private:
  // IP 与 用户名的映射
  static std::unordered_map<std::string, std::string> users;
  // 保护 users 的互斥锁
  static std::mutex users_mutex;
  // IP 与 客户端信息的映射
  static std::unordered_map<std::string, Client> clients;
  // 保护 clients 的互斥锁
  static std::mutex clients_mutex;
  // 登陆状态
  static std::unordered_map<std::string, bool> login_status;
  // 保护 login_status 的互斥锁
  static std::mutex login_status_mutex;
};

} // namespace ftp