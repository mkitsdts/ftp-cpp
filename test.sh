#!/bin/bash
# 简单的 FTP curl 测试脚本

SERVER_IP=${1:-"127.0.0.1"}
SERVER_PORT=${2:-"21"}
TEST_USER="root"
TEST_PASS="root"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}FTP Curl 测试脚本${NC}"
echo -e "服务器: ${SERVER_IP}:${SERVER_PORT}"
echo -e "用户名: ${TEST_USER} / 密码: ${TEST_PASS}"
echo "========================================"

# 创建测试文件
echo "Hello FTP World!" > test_upload.txt
echo "测试时间: $(date)" >> test_upload.txt

# 1. 测试连接和列出目录
echo -e "\n${YELLOW}1. 测试连接并列出目录${NC}"
curl -v --connect-timeout 10 ftp://${TEST_USER}:${TEST_PASS}@${SERVER_IP}:${SERVER_PORT}/

# 2. 测试上传文件
echo -e "\n${YELLOW}2. 测试上传文件${NC}"
curl -v --connect-timeout 10 -T test_upload.txt ftp://${TEST_USER}:${TEST_PASS}@${SERVER_IP}:${SERVER_PORT}/

# 3. 再次列出目录（查看上传的文件）
echo -e "\n${YELLOW}3. 验证上传结果${NC}"
curl -v --connect-timeout 10 ftp://${TEST_USER}:${TEST_PASS}@${SERVER_IP}:${SERVER_PORT}/

# 4. 测试下载文件
echo -e "\n${YELLOW}4. 测试下载文件${NC}"
curl -v --connect-timeout 10 -o downloaded_file.txt ftp://${TEST_USER}:${TEST_PASS}@${SERVER_IP}:${SERVER_PORT}/test_upload.txt

# 5. 验证下载的文件
if [ -f downloaded_file.txt ]; then
    echo -e "\n${GREEN}✓ 文件下载成功${NC}"
    echo "下载文件内容:"
    cat downloaded_file.txt
else
    echo -e "\n${RED}✗ 文件下载失败${NC}"
fi

# 6. 测试被动模式
echo -e "\n${YELLOW}5. 测试被动模式${NC}"
curl -v --connect-timeout 10 --ftp-pasv ftp://${TEST_USER}:${TEST_PASS}@${SERVER_IP}:${SERVER_PORT}/

# 7. 测试主动模式
echo -e "\n${YELLOW}6. 测试主动模式${NC}"
curl -v --connect-timeout 10 --disable-epsv --ftp-port - ftp://${TEST_USER}:${TEST_PASS}@${SERVER_IP}:${SERVER_PORT}/

# 清理
rm -f test_upload.txt downloaded_file.txt

echo -e "\n${BLUE}测试完成${NC}"