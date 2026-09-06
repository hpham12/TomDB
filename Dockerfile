FROM ubuntu:26.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gdb \
    valgrind \
    clang-tidy \
    python3-pip \
    && pip3 install --break-system-packages gcovr \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace