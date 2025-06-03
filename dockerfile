# Stage 1: Build the application
FROM alpine:latest AS builder

LABEL stage="builder"

RUN apk update && apk add --no-cache \
    build-base \
    git \
    curl \
    bash

RUN sh <(curl -sSf https://xmake.io/shget.text)
ENV PATH="/root/.local/bin:${PATH}"

WORKDIR /app

COPY xmake.lua .

COPY src/ ./src/

RUN xmake config --mode=release --root -y && \
    xmake build -P . --root -y

RUN mkdir -p /app/dist

RUN find build -type f -name ftp -executable -exec cp {} /app/dist/ftp \;

# Stage 2: Create the runtime image
FROM alpine:latest

LABEL stage="runtime"

RUN apk add --no-cache libstdc++

WORKDIR /app

COPY --from=builder /app/dist/ftp .

# 创建用于FTP测试的文件和目录
RUN mkdir -p /app/file && \
    echo "这是一个FTP测试文件。" > /app/files/test_file.txt && \
    echo "这是另一个示例文档。" > /app/files/sample_document.md

EXPOSE 21/tcp
EXPOSE 21000-21010/tcp

RUN chmod +x ./ftp

CMD ["./ftp"]
