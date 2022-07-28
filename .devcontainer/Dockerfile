# +-----------------------------+
# | BASE IMAGE             ~30MB|        See https://hub.docker.com/_/debian
# +-----------------------------+

FROM debian:sid-slim

# +-----------------------------+
# | METADATA                    |
# +-----------------------------+

LABEL maintainer Jens A. Koch <jakoch@web.de>
LABEL version="2022-01-27"
LABEL description="C++ DevBox (LLVM 13 & GCC 11) (CMake & VCPKG) (zsh + p10k)"
LABEL base.image="debian:sid-slim"

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
    zsh \
    # iproute2 procps less gnupg
    fontconfig
    # optional dependencies
    doxygen graphviz ccache cppcheck

# +-----------------------------+
# | CMAKE - Latest Version      |
# +-----------------------------+

# list downloads
# curl -s https://api.github.com/repos/Kitware/CMake/releases/latest | jq -r '.assets[] | .browser_download_url'

RUN version="$(curl -s https://api.github.com/repos/Kitware/CMake/releases/latest | jq -r '.tag_name' | cut -c 2-)"; echo "Latest Version: $version" && \
    url="https://github.com/Kitware/CMake/releases/download/v$version/cmake-$version-linux-x86_64.sh"; echo "Download URL: $url" && \
    wget $url && \
    mkdir /opt/cmake && \
    sudo sh cmake-$version-linux-x86_64.sh --prefix=/opt/cmake --skip-license && \
    ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake

# +-----------------------------+
# | LLVM                        |
# +-----------------------------+

# set repo key
RUN curl -sS https://apt.llvm.org/llvm-snapshot.gpg.key | (OUT=$(apt-key add - 2>&1) || echo $OUT)

# https://apt.llvm.org/unstable/pool/main/l/
RUN add-apt-repository "deb https://apt.llvm.org/unstable/    llvm-toolchain-14  main" && \
    apt-get -qq update && \
    apt-get install -qqy --no-install-recommends \
    clang-14 lldb-14 lld-14 clangd-14 clang-format-14 clang-tidy-14 libc++-dev libc++abi-dev

ENV PATH="/usr/lib/llvm-14/bin:/usr/lib/llvm-14/include:${PATH}"
ENV LD_LIBRARY_PATH="/usr/lib/llvm-14/lib:${LD_LIBRARY_PATH}"

ENV CC=/usr/bin/clang-14
ENV CXX=/usr/bin/clang++-14

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
ENV VCPKG_USE_SYSTEM_BINARIES=1

RUN mkdir -p $VCPKG_PATH && \
    git clone --depth=1 https://github.com/Microsoft/vcpkg.git $VCPKG_PATH && \
    cd $VCPKG_PATH && \
    ./bootstrap-vcpkg.sh -disableMetrics

# +-----------------------------+
# | Powerline Fonts             |
# +-----------------------------+

RUN git clone https://github.com/powerline/fonts.git --depth=1 /tmp/fonts && \
    # install MesloLGS_NF
    cd /tmp/fonts && \
    mkdir MesloLGS_NF && cd MesloLGS_NF && \
    wget https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Regular.ttf && \
    wget https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Bold.ttf && \
    wget https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Italic.ttf && \
    wget https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Bold%20Italic.ttf && \
    cd .. && \
    ./install.sh && \
    # clean-up a bit
    cd .. && \
    rm -rf /tmp/fonts && \
    # refresh fontcache
    fc-cache -f

# +-----------------------------+
# | ZSH + Theme                 |
# +-----------------------------+

# set terminal colors and theme
ENV TERM xterm-256color
ENV ZSH_THEME powerlevel10k/powerlevel10k

# run ohmyzsh installer
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended

# copy theme config
# you might run "p10k configure" to configure using a wizard
COPY .p10k.zsh /root/.p10k.zsh

# install theme
RUN git clone --depth=1 https://github.com/romkatv/powerlevel10k.git ~/.oh-my-zsh/themes/powerlevel10k
RUN sed -i 's/ZSH_THEME="robbyrussell"/ZSH_THEME="powerlevel10k\/powerlevel10k"/g' ~/.zshrc && \
    echo '[[ ! -f ~/.p10k.zsh ]] || source ~/.p10k.zsh' >> ~/.zshrc && \
    echo "POWERLEVEL9K_DISABLE_CONFIGURATION_WIZARD=true" >> ~/.zshrc && \
    echo "export LANG=en_US.UTF-8" >> ~/.zshrc

RUN git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting
RUN git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions

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

#ENV LANG=de_DE.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

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
