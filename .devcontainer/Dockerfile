# +-----------------------------+
# | BASE IMAGE             ~27MB|        See https://hub.docker.com/_/debian
# +-----------------------------+

FROM debian:sid-slim

# +-----------------------------+
# | METADATA                    |
# +-----------------------------+

# MAINTAINER Jens A. Koch <jakoch@web.de>

# LABEL base.image="debian:sid-slim"

# +-----------------------------+
# | ARGS                        |
# +-----------------------------+

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# +-----------------------------+
# | ENV                         |
# +-----------------------------+

ENV DEBIAN_FRONTEND=noninteractive

# Fallback for LANG and LC_ALL
# Please see "Set Timezone + Locale" section below
#
#ENV LANG C.UTF-8
#ENV LC_ALL C.UTF-8

# +-----------------------------+
# | PRE-REQUISITE/INIT PACKAGES |
# +-----------------------------+

# avoid debconf delaying package configuration, since apt-utils is not installed
RUN apt-get update && \
    apt-get -y install --no-install-recommends apt-utils dialog 2>&1

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    gnupg2 software-properties-common lsb-release \
    build-essential cppcheck valgrind ccache wget git jq \
    # required by Visual Studio
    openssh-server g++ gdb make ninja-build rsync zip sudo \
    # required by VCPKG
    curl unzip tar pkg-config \
    # optional downloader for VCPKG
    aria2 \
    # required by LLVM
    zlib1g-dev \
    # locale
    locales \
    # shell
    zsh
    # iproute2 procps less gnupg

# +-----------------------------+
# | CMAKE - Latest Version      |
# +-----------------------------+

RUN wget https://cmake.org/files/v3.21/cmake-3.21.1-linux-x86_64.sh && \
    mkdir /opt/cmake && \
    sudo sh cmake-3.21.1-linux-x86_64.sh --prefix=/opt/cmake --skip-license && \
    ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake

# +-----------------------------+
# | LLVM                        |
# +-----------------------------+

# set repo key
RUN curl -sS https://apt.llvm.org/llvm-snapshot.gpg.key | (OUT=$(apt-key add - 2>&1) || echo $OUT)

RUN add-apt-repository "deb https://apt.llvm.org/unstable/    llvm-toolchain-13  main" && \
    apt-get -qq update && \
    apt-get install -qqy --no-install-recommends \
    clang-13 lldb-13 lld-13 clangd-13 clang-format-13 clang-tidy-13 libc++-dev libc++abi-dev

ENV PATH="/usr/lib/llvm-13/bin:/usr/lib/llvm-13/include:${PATH}"
ENV LD_LIBRARY_PATH="/usr/lib/llvm-13/lib:${LD_LIBRARY_PATH}"

ENV CC=/usr/bin/clang-13
ENV CXX=/usr/bin/clang++-13

# +-----------------------------+
# | vcpkg                 ~225MB|
# +-----------------------------+

# Note: vcpkg requires curl, unzip, tar, pkg-config (optional cmake)

ENV VCPKG_ROOT=/opt/vcpkg
ENV VCPKG_PATH=/opt/vcpkg/bin
ENV PATH="${PATH}:${VCPKG_PATH}"
ENV VCPKG_TRIPLET=x64-linux

# Use installed binaries from the system.
# Do not download latest version of CMake and Ninja during vcpkg bootstrap!
ENV VCPKG_FORCE_SYSTEM_BINARIES=1

RUN mkdir -p $VCPKG_PATH && \
    git clone --depth=1 https://github.com/Microsoft/vcpkg.git $VCPKG_PATH && \
    cd $VCPKG_PATH && \
    ./bootstrap-vcpkg.sh -disableMetrics -useSystemBinaries && \
    sudo /opt/vcpkg/bin/vcpkg integrate install

# install default packages globally
#RUN sudo /opt/vcpkg/bin/vcpkg install gtest catch2 spdlog fmt nlohmann-json

# +-----------------------------+
# | Powerline Fonts             |
# +-----------------------------+

RUN git clone --depth=1 https://github.com/powerline/fonts.git && \
    sh fonts/install.sh && \
    rm -rf fonts

# +-----------------------------+
# | ZSH + Theme                 |
# +-----------------------------+

# set terminal colors and theme
ENV TERM xterm-256color
ENV ZSH_THEME powerlevel10k/powerlevel10k

# get and run installer
RUN wget https://github.com/robbyrussell/oh-my-zsh/raw/master/tools/install.sh -O - | zsh || true

# install theme
RUN git clone --depth=1 https://github.com/romkatv/powerlevel10k.git ~/.oh-my-zsh/themes/powerlevel10k
RUN sed -i 's/ZSH_THEME="robbyrussell"/ZSH_THEME="powerlevel10k\/powerlevel10k"/g' ~/.zshrc && \
    echo '[[ ! -f ~/.p10k.zsh ]] || source ~/.p10k.zsh' >> ~/.zshrc

# copy theme config
# you might run "p10k configure" to configure using a wizard
COPY .p10k.zsh /root/

# +-----------------------------+
# | CLEANUP                     |
# +-----------------------------+

RUN apt-get autoremove -y && \
    apt-get clean autoclean && \
    rm -rf /var/lib/apt/lists/*

# +-----------------------------+
# | SSH SERVER SETUP            |
# +-----------------------------+

# Setup SSHD server
RUN mkdir -p /var/run/sshd && \
    echo 'PasswordAuthentication yes' >> /etc/ssh/sshd_config && \
    ssh-keygen -A

# Tweak non-root user
RUN if [ "$USER_GID" != "1000" ] || [ "$USER_UID" != "1000" ]; then \
        groupmod --gid $USER_GID $USERNAME && \
        usermod --uid $USER_UID --gid $USER_GID $USERNAME && \
        chown -R $USER_UID:$USER_GID /home/$USERNAME; \
    fi

# +-----------------------------+
# | Set Locale + Timezone       |
# +-----------------------------+

ENV TZ=Europe/Berlin

RUN echo $TZ > /etc/timezone && \
    dpkg-reconfigure tzdata && \
    sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    sed -i -e 's/# de_DE.UTF-8 UTF-8/de_DE.UTF-8 UTF-8/' /etc/locale.gen && \
    echo 'LANG="de_DE.UTF-8"' > /etc/default/locale && \
    dpkg-reconfigure locales && \
    update-locale LANG=de_DE.UTF-8

ENV LANG=de_DE.UTF-8

RUN echo "export PATH=$PATH" > /etc/environment

# +-----------------------------+
# | USER                        |
# +-----------------------------+

# [Optional] Set the default user. Omit if you want to keep the default as root.
#USER $USERNAME
#WORKDIR /home/$USERNAME/dev
#VOLUME /home/$USERNAME/dev

# +-----------------------------+
# | PORT                        |
# +-----------------------------+

#EXPOSE 22

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=dialog
