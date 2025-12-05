FROM ubuntu:22.04

# 设置非交互式安装
ENV DEBIAN_FRONTEND=noninteractive

# 安装必要的软件
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# 安装Python依赖
RUN pip3 install flask flask-cors

# 创建工作目录
WORKDIR /app

# 复制项目文件
COPY . /app/

# 编译C++程序
RUN make clean && make

# 创建必要的目录
RUN mkdir -p /app/uploads /app/test_results /app/templates

# 设置权限
RUN chmod +x /app/hotwords
RUN chmod 755 /app

# 暴露端口
EXPOSE 5000

# 启动Flask服务
CMD ["python3", "web_server.py"]
