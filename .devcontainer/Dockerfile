FROM ubuntu:jammy

# Add vscode user with same UID and GID as your host system
# (modified from https://code.visualstudio.com/remote/advancedcontainers/add-nonroot-user#_creating-a-nonroot-user)
# ubuntu now has a 1000 user ubuntu
ARG USERNAME=ubuntu
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

RUN apt-get update && apt-get upgrade -y \
    && rm -rf /var/lib/apt/lists/*


RUN apt-get update && apt-get install --no-install-recommends -y \
    sudo \
    git \
    wget \
    ca-certificates \
    libpoco-dev \
    g++ \
    cmake \
    libeigen3-dev \
    libssl-dev \
    nlohmann-json3-dev \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install nlohmann/json header
RUN wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp -O /usr/local/include/nlohmann/json.hpp

RUN echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Switch from root to user
USER $USERNAME

# make sure folders exist
WORKDIR /workspaces/nano-vectordb