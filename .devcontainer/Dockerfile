FROM mcr.microsoft.com/vscode/devcontainers/cpp:debian

# Install required packages
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    python3 \
    python3-pip \
    git \
    curl \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install arduino-cli
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Set up arduino-cli config
RUN arduino-cli config init

# Add arduino-cli to PATH
ENV PATH="/usr/local/bin:${PATH}"

# Create workspace directory
WORKDIR /workspace

# Copy arduino-cli configuration
COPY arduino-cli.yaml /root/.arduino15/arduino-cli.yaml

# Install build essentials
RUN apt-get update && apt-get install -y build-essential

# Install common Arduino libraries
RUN arduino-cli lib install \
    "OneWire" \
    "DallasTemperature" \
    "Adafruit BusIO" \
    "Adafruit Unified Sensor" \
    "DHT sensor library" \
    "WiFiManager" \
    "ArduinoJson" \
    "PubSubClient" \
    "ESP8266WiFi" \
    "ESP32" \
    "Wire" \
    "SPI" \
    "FastLED" \
    "NTPClient" \
    "AsyncTCP" \
    "ESPAsyncTCP" \
    "ESPAsyncWebServer"

# Verify library installation
RUN arduino-cli lib list

# Copy update script
COPY update-libraries.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/update-libraries.sh

# Add a weekly cron job to update libraries
RUN (crontab -l 2>/dev/null; echo "0 0 * * 0 /usr/local/bin/update-libraries.sh > /var/log/library-updates.log 2>&1") | crontab -

# Add aliases for build operations
RUN echo 'alias arduino-build="./build.sh build"' >> /home/vscode/.bashrc && \
    echo 'alias arduino-test="./build.sh test"' >> /home/vscode/.bashrc && \
    echo 'alias arduino-build-test="./build.sh all"' >> /home/vscode/.bashrc