# Use the base image for C++ development
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
    build-essential \
    dos2unix \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Add vscode user to the docker group
RUN usermod -aG docker vscode

# Create necessary directories and set permissions
RUN mkdir -p /home/vscode/.local/share/CMakeTools \
    && mkdir -p /home/vscode/.ssh \
    && mkdir -p /home/vscode/.config/fish \
    && chown -R vscode:vscode /home/vscode/.local /home/vscode/.ssh /home/vscode/.config \
    && chmod 700 /home/vscode/.ssh

# Install arduino-cli
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Set up arduino-cli config
RUN arduino-cli config init

# Add arduino-cli to PATH
ENV PATH="/usr/local/bin:${PATH}"

# Create workspace directory
WORKDIR /workspace

# Copy arduino-cli configuration (customize to your actual path)
COPY .devcontainer/arduino-cli.yaml /root/.arduino15/arduino-cli.yaml

# Install Arduino cores for ESP8266 and ESP32
RUN arduino-cli core update-index --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json,https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json \
    && arduino-cli core install esp8266:esp8266 --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json \
    && arduino-cli core install esp32:esp32 --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

# Install required Arduino libraries
RUN arduino-cli lib install "OneWire" "ArduinoUnit"

# Verify library installation
RUN arduino-cli lib list

# Copy update script and set permissions
COPY .devcontainer/update-libraries.sh /usr/local/bin/
RUN dos2unix /usr/local/bin/update-libraries.sh \
    && chmod +x /usr/local/bin/update-libraries.sh

# Add aliases for build operations (for Bash)
RUN echo 'alias arduino-build="./build.sh build"' >> /home/vscode/.bashrc \
    && echo 'alias arduino-test="./build.sh test"' >> /home/vscode/.bashrc \
    && echo 'alias arduino-build-test="./build.sh all"' >> /home/vscode/.bashrc

# Add a welcome message to .bashrc
RUN echo '\n# Welcome to the dev container! Here are some useful aliases:' >> /home/vscode/.bashrc \
    && echo 'echo "  - arduino-build: Build the project"' >> /home/vscode/.bashrc \
    && echo 'echo "  - arduino-test: Run tests for the project"' >> /home/vscode/.bashrc \
    && echo 'echo "  - arduino-build-test: Build and test the project"' >> /home/vscode/.bashrc

# Add aliases and welcome message for Fish shell
RUN echo 'alias arduino-build="./build.sh build"' >> /home/vscode/.config/fish/config.fish \
    && echo 'alias arduino-test="./build.sh test"' >> /home/vscode/.config/fish/config.fish \
    && echo 'alias arduino-build-test="./build.sh all"' >> /home/vscode/.config/fish/config.fish \
    && echo '\n# Welcome to the dev container! Here are some useful aliases:' >> /home/vscode/.config/fish/config.fish \
    && echo 'echo "  - arduino-build: Build the project"' >> /home/vscode/.config/fish/config.fish \
    && echo 'echo "  - arduino-test: Run tests for the project"' >> /home/vscode/.config/fish/config.fish \
    && echo 'echo "  - arduino-build-test: Build and test the project"' >> /home/vscode/.config/fish/config.fish

# Generate SSH keys and set proper ownership and permissions
RUN if [ ! -f /home/vscode/.ssh/id_rsa ]; then \
        ssh-keygen -t rsa -b 4096 -N "" -C "devcontainer@local" -f /home/vscode/.ssh/id_rsa \
        && chmod 600 /home/vscode/.ssh/id_rsa \
        && chmod 644 /home/vscode/.ssh/id_rsa.pub \
        && chown vscode:vscode /home/vscode/.ssh/id_rsa /home/vscode/.ssh/id_rsa.pub ; \
    fi

# Ensure the Docker socket has the correct permissions
RUN echo '#!/bin/sh\nsudo chmod 666 /var/run/docker.sock\nexec "$@"' > /usr/local/bin/docker-entrypoint.sh \
    && chmod +x /usr/local/bin/docker-entrypoint.sh

# Set the entrypoint to fix permissions and start the container
ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["sleep", "infinity"]