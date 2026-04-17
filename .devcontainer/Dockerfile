FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=Asia/Seoul \
    LANG=ko_KR.UTF-8 \
    LANGUAGE=ko_KR.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
    locales tzdata build-essential sudo gdb vim curl git wget \
    python3 cmake make gcc valgrind telnet net-tools iproute2 \
    && locale-gen ko_KR.UTF-8 \
    && update-locale LANG=ko_KR.UTF-8 \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash jungle && usermod -aG sudo jungle
RUN echo "jungle ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/jungle
# 암호 설정
# RUN echo 'jungle:jungle' | chpasswd

USER jungle
WORKDIR /home/jungle
