FROM ubuntu:26.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gdb \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace