FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    libgl1 \
    libegl1 \
    libxkbcommon-x11-0 \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xkb1 \
    libxcb-shape0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S . -B build \
    -DTIVIDY_BUILD_DESKTOP=ON \
    -DTIVIDY_BUILD_TESTS=OFF

RUN cmake --build build --target tividy_desktop -j4

ENV QT_QPA_PLATFORM=xcb

CMD ["./build/tividy_desktop"]

