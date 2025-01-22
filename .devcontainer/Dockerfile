FROM mcr.microsoft.com/vscode/devcontainers/cpp:debian

# Install required packages
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    python3 \
    python3-pip \
    git \
    curl \
    fish \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /home/vscode/.local/share/CMakeTools \
&& chown -R vscode:vscode /home/vscode/.local/share/CMakeTools

RUN mkdir -p /home/vscode/.ssh \
    && chown vscode:vscode /home/vscode/.ssh \
    && chmod 700 /home/vscode/.ssh

# Install arduino-cli
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Set up arduino-cli config
RUN arduino-cli config init

# Add arduino-cli to PATH
ENV PATH="/usr/local/bin:${PATH}"

# Create workspace directory
WORKDIR /workspace

# Copy arduino-cli configuration (customise to your actual path)
COPY arduino-cli.yaml /root/.arduino15/arduino-cli.yaml

# Install build essentials
RUN apt-get update && apt-get install -y build-essential && rm -rf /var/lib/apt/lists/*

# (Optional) Install Arduino cores for ESP8266 and ESP32 if needed
RUN arduino-cli core install esp8266:esp8266 esp32:esp32

# Install only required dependencies for DallasTemperature library and others
RUN arduino-cli lib install \
    "OneWire" \
    "ArduinoUnit"  # For testing

# Verify library installation
RUN arduino-cli lib list

# Copy update script
COPY update-libraries.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/update-libraries.sh

# Add aliases for build operations (for Bash)
RUN echo 'alias arduino-build="./build.sh build"' >> /home/vscode/.bashrc && \
    echo 'alias arduino-test="./build.sh test"' >> /home/vscode/.bashrc && \
    echo 'alias arduino-build-test="./build.sh all"' >> /home/vscode/.bashrc

# Add a welcome message to .bashrc
RUN echo '\n# Welcome to the dev container! Here are some useful aliases:' >> /home/vscode/.bashrc && \
    echo 'echo "  - arduino-build: Build the project"' >> /home/vscode/.bashrc && \
    echo 'echo "  - arduino-test: Run tests for the project"' >> /home/vscode/.bashrc && \
    echo 'echo "  - arduino-build-test: Build and test the project"' >> /home/vscode/.bashrc

# (Optional) Add fish-specific configuration if desired
# For example, you might add an alias file or welcome message for fish:
RUN mkdir -p /home/vscode/.config/fish && \
    echo 'set -gx PATH /usr/local/bin $PATH' >> /home/vscode/.config/fish/config.fish && \
    echo '# Welcome to the Fish shell inside the dev container!' >> /home/vscode/.config/fish/config.fish

# Generate SSH keys and set proper ownership and permissions
RUN if [ ! -f /home/vscode/.ssh/id_rsa ]; then \
        ssh-keygen -t rsa -b 4096 -N "" -C "devcontainer@local" -f /home/vscode/.ssh/id_rsa && \
        chmod 600 /home/vscode/.ssh/id_rsa && \
        chmod 644 /home/vscode/.ssh/id_rsa.pub && \
        chown vscode:vscode /home/vscode/.ssh/id_rsa /home/vscode/.ssh/id_rsa.pub ; \
    fi
