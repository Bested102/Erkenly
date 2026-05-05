FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    libboost-all-dev \
    libhiredis-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app

RUN rm -rf build server/build

RUN cmake -S server -B server/build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build server/build

RUN cp server/data.json server/build/data.json

EXPOSE 5555

CMD ["./server/build/parking_server"]