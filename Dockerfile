
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN g++ -std=c++11 -Iinclude src/main.cpp -o tividy

CMD ["./tividy"]
