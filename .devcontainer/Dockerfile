FROM mcr.microsoft.com/vscode/devcontainers/cpp:debian

# Install required packages
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    python3 \
    python3-pip \
    git \
    curl \
    fish \
    docker.io \
    sudo \
    build-essential \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Add user to docker group
RUN usermod -aG docker vscode

# Create directories and set permissions
RUN mkdir -p /home/vscode/.local/share/CMakeTools \
    && chown -R vscode:vscode /home/vscode/.local/share/CMakeTools \
    && mkdir -p /home/vscode/.ssh \
    && chown vscode:vscode /home/vscode/.ssh \
    && chmod 700 /home/vscode/.ssh

# Install Arduino CLI
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh \
    && arduino-cli config init

ENV PATH="/usr/local/bin:${PATH}"
WORKDIR /workspace

# Copy Arduino config and update script
COPY arduino-cli.yaml /root/.arduino15/arduino-cli.yaml
COPY update-libraries.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/update-libraries.sh

# Install Arduino cores and libraries
RUN arduino-cli core install esp8266:esp8266 esp32:esp32 \
    && arduino-cli lib install "OneWire" "ArduinoUnit" \
    && arduino-cli lib list

# Configure shells
RUN echo 'alias arduino-build="./build.sh build"\nalias arduino-test="./build.sh test"\nalias arduino-build-test="./build.sh all"' >> /home/vscode/.bashrc \
    && echo '\n# Welcome to the dev container! Here are some useful aliases:\necho "  - arduino-build: Build the project"\necho "  - arduino-test: Run tests for the project"\necho "  - arduino-build-test: Build and test the project"' >> /home/vscode/.bashrc \
    && mkdir -p /home/vscode/.config/fish \
    && echo 'set -gx PATH /usr/local/bin $PATH\n# Welcome to the Fish shell inside the dev container!' >> /home/vscode/.config/fish/config.fish

# Generate SSH keys
RUN if [ ! -f /home/vscode/.ssh/id_rsa ]; then \
        ssh-keygen -t rsa -b 4096 -N "" -C "devcontainer@local" -f /home/vscode/.ssh/id_rsa && \
        chmod 600 /home/vscode/.ssh/id_rsa && \
        chmod 644 /home/vscode/.ssh/id_rsa.pub && \
        chown vscode:vscode /home/vscode/.ssh/id_rsa /home/vscode/.ssh/id_rsa.pub ; \
    fi

# Docker-in-Docker setup
RUN echo '#!/bin/sh\nchmod 666 /var/run/docker.sock\nexec "$@"' > /usr/local/bin/docker-entrypoint.sh \
    && chmod +x /usr/local/bin/docker-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["sleep", "infinity"]