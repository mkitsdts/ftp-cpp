# ftp-cpp

cpp 实现的简易 ftp 服务器，支持被动、主动文件传输，访问文件目录等常规操作

## 配置

所有配置都在 configs.hpp 文件中

- MAX_CONNECTIONS   最大客户端连接数

- BUFFER_SIZE       缓冲区大小

- PORT              监听端口号

- ROOT_PATH         根目录，客户端 list 命令传递的路径参数均为基于此根目录的相对目录

- USER_INFO         存储可登陆的帐号密码

## 环境

- xmake 环境
- 编译器支持 c++17 及以上
- 符合 posix 接口设计的操作系统

## 部署

编译后直接启动二进制文件即可，需要注意 21 端口不被占用或自行在 configs.hpp 文件中配置其他端口

## 

通过 curl 测试，理论上也能通过 ftp 客户端