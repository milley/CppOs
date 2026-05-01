# Docker 开发环境 - 提供交叉编译工具链
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 安装编译工具
RUN apt-get update && apt-get install -y \
    nasm \
    gcc \
    make \
    binutils \
    gdb \
    xorriso \
    mtools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# 默认命令
CMD ["/bin/bash"]
