From ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Zurich

WORKDIR /usr/src

RUN apt-get update && apt-get install -y software-properties-common
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-get update && \
    apt-get install -y \
        build-essential gcc-11 g++-11 make \
        libeigen3-dev libopenimageio-dev libceres-dev librocksdb-dev rapidjson-dev
RUN apt-get install -y \
    libjpeg-dev libpng-dev
RUN apt-get install -y \
    git vim
RUN apt-get install -y \
    libgl1-mesa-dev \
    freeglut3-dev

# Update cmake
RUN apt-get remove -y cmake
RUN apt-get update -y && apt-get install -y curl
RUN curl -s "https://cmake.org/files/v3.19/cmake-3.19.4-Linux-x86_64.tar.gz" | tar --strip-components=1 -xz -C /usr/local


