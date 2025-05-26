#include "parser.hpp"
#include <iostream>
using namespace ftp;
Command Parser::parse(std::string &command) {
  std::cout << "Parsing command: " << command.substr(0, 4) << std::endl;
  std::cout << "Command: " << command.substr(5) << std::endl;
  if (!is_valid_command(command)) {
    return Command::ERROR;
  }
  Upper(command);
  if (command.substr(0, 4) == "USER") {
    trim_ftp_command(command);
    return Command::USER;
  } else if (command.substr(0, 4) == "PASS") {
    trim_ftp_command(command);
    return Command::PASS;
  } else if (command.substr(0, 4) == "LIST") {
    trim_ftp_command(command);
    return Command::LIST;
  } else if (command.substr(0, 4) == "QUIT") {
    trim_ftp_command(command);
    return Command::QUIT;
  } else if (command.substr(0, 4) == "RETR") {
    trim_ftp_command(command);
    return Command::RETR;
  } else if (command.substr(0, 3) == "GET") {
    trim_ftp_command(command);
    return Command::GET;
  } else if (command.substr(0, 3) == "PWD") {
    trim_ftp_command(command);
    return Command::PWD;
  } else if (command.substr(0, 4) == "EPSV") {
    trim_ftp_command(command);
    return Command::EPSV;
  } else if (command.substr(0, 4) == "PASV") {
    trim_ftp_command(command);
    return Command::PASV;
  } else if (command.substr(0, 3) == "LCD") {
    trim_ftp_command(command);
    return Command::LCD;
  } else if (command.substr(0, 4) == "SYST") {
    trim_ftp_command(command);
    return Command::SYST;
  } else if (command.substr(0, 4) == "TYPE") {
    trim_ftp_command(command);
    return Command::TYPE;
  } else if (command.substr(0, 4) == "PORT") {
    trim_ftp_command(command);
    return Command::PORT;
  }
  std::cout << "Unknown command: " << command << std::endl;
  return Command::ERROR;
}

void Parser::trim_ftp_command(std::string &str) {
  // 移除末尾的 \r\n
  while (!str.empty() && (str.back() == '\r' || str.back() == '\n')) {
    str.pop_back();
  }
  if (str.size() < 4) {
    str = "";
    return;
  }
  for (int i = 0; i < str.size(); i++) {
    if (str[i] == ' ') {
      str = str.substr(i + 1);
      break;
    }
  }
}

void Parser::Upper(std::string &str) {
  for (int i = 0; i < str.size(); i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] -= 32;
    } else if (str[i] == ' ') {
      break;
    }
  }
}

bool Parser::is_valid_command(const std::string &command) {
  // 检查命令是否合法
  if (command.empty()) {
    return false;
  }
  int space_count = 0;
  for (char c : command) {
    if (c == ' ') {
      space_count++;
      if (space_count > 1) {
        return false; // 只允许一个空格
      }
    }
  }
  return true;
}

std::pair<std::string, int> Parser::parse_path(std::string_view ip) {
  // 把逗号ip转换成点分十进制
  std::string path;
  int first = 0;
  int second = 0;

  int times = 0;
  for (auto c : ip) {
    if (times < 4) {
      if (c == ',') {
        times++;
        if (times == 4) {
          continue;
        }
        path += '.';
      } else if (c >= '0' && c <= '9') {
        path += c;
      } else {
        return {"", -1}; // 无效的IP格式
      }
    } else {
      if (times == 4) {
        if (c == ',') {
          times++;
          first = first * 256;
          continue; // 跳过逗号
        }
        if (c >= '0' && c <= '9') {
          first = first * 10 + (c - '0');
        } else {
          return {"", -1}; // 无效的端口格式
        }
      } else {
        if (c >= '0' && c <= '9') {
          second = second * 10 + (c - '0');
        } else {
          return {"", -1}; // 无效的端口格式
        }
      }
    }
  }
  return {path, first + second}; // 返回点分十进制IP和端口号
}