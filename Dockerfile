# Linux development environment for the EPL421 io_uring / liburing presentation.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        liburing-dev \
        pkg-config \
        gdb \
        strace \
        valgrind \
        git \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
CMD ["bash"]
