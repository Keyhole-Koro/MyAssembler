# Use an official GCC image as the base
FROM gcc:latest

RUN apt update && apt install -y \
    gdb \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /app

# Copy the project files into the container
COPY . .