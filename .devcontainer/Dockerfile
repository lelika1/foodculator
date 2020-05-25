FROM alpine:latest as BASE
RUN apk --no-cache add \
    sqlite-dev \
    libstdc++ \
    zlib-dev \
    openssl-dev \
    curl-dev \
    boost-dev \
    build-base \
    clang \
    python \
    cmake \
    git

RUN mkdir /build

ENV CC=/usr/bin/clang 
ENV CXX=/usr/bin/clang++
