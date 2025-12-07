# 基于官方最新 Ubuntu
FROM ubuntu:24.04

# 设置非交互模式
ENV DEBIAN_FRONTEND=noninteractive

# 更新并安装常用工具
RUN apt update && apt install -y \
    build-essential \
    cmake \
    git \
    curl \
    wget \
    python3 \
    python3-pip \
    python3-venv \
    pipx \
    clang \
    clangd \
    gdb \
    ninja-build \
    && apt clean && rm -rf /var/lib/apt/lists/*

# 安装 Conan（使用 pipx 安全安装）
RUN pipx ensurepath && pipx install conan
ENV PATH="/root/.local/bin:${PATH}"

# 创建工作目录
WORKDIR /workspace

# 默认命令
CMD ["/bin/bash"]
