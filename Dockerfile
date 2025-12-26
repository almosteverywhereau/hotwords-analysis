FROM ubuntu:22.04


ENV DEBIAN_FRONTEND=noninteractive


RUN apt-get update && apt-get install -y \
    g++ \
    make \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*


RUN pip3 install flask flask-cors

WORKDIR /app

COPY . /app/

RUN make clean && make

RUN mkdir -p /app/uploads /app/test_results /app/templates

RUN chmod +x /app/hotwords
RUN chmod 755 /app

EXPOSE 5000

CMD ["python3", "web_server.py"]
