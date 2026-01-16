# syntax=docker/dockerfile:1
FROM ubuntu:24.04

ARG CMAKE_VERSION=3.31.2
ENV DEBIAN_FRONTEND=noninteractive
ENV CMAKE_BUILD_PARALLEL_LEVEL=1

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    git \
    build-essential \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN set -eux; \
    arch="$(uname -m)"; \
    case "$arch" in \
      x86_64) cmake_arch="linux-x86_64" ;; \
      aarch64|arm64) cmake_arch="linux-aarch64" ;; \
      *) echo "Unsupported arch: $arch" >&2; exit 1 ;; \
    esac; \
    url="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-${cmake_arch}.tar.gz"; \
    curl -fsSL "$url" -o /tmp/cmake.tar.gz; \
    tar -C /opt -xzf /tmp/cmake.tar.gz; \
    rm /tmp/cmake.tar.gz; \
    ln -s "/opt/cmake-${CMAKE_VERSION}-${cmake_arch}/bin/"* /usr/local/bin/

WORKDIR /work
COPY . /work

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

RUN install -m 0755 docker/run.sh /usr/local/bin/run.sh

ENTRYPOINT ["/usr/local/bin/run.sh"]
CMD ["main"]
