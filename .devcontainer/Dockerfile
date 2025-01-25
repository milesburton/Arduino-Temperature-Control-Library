FROM mcr.microsoft.com/vscode/devcontainers/cpp:debian

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
      && rm -rf /var/lib/apt/lists/* \
      && usermod -aG docker vscode

RUN mkdir -p /home/vscode/.local/share/CMakeTools \
      && mkdir -p /home/vscode/.ssh \
      && mkdir -p /home/vscode/.config/fish \
      && chown -R vscode:vscode /home/vscode/.local /home/vscode/.ssh /home/vscode/.config \
      && chmod 700 /home/vscode/.ssh

RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh \
      && arduino-cli config init

ENV PATH="/usr/local/bin:${PATH}"
WORKDIR /workspace

COPY .devcontainer/arduino-cli.yaml /root/.arduino15/arduino-cli.yaml
RUN arduino-cli core update-index --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json,https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json \
      && arduino-cli core install --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json,https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json esp8266:esp8266 esp32:esp32 \
      && arduino-cli lib install "OneWire" "ArduinoUnit"

COPY .devcontainer/update-libraries.sh /usr/local/bin/
RUN dos2unix /usr/local/bin/update-libraries.sh && \
      chmod +x /usr/local/bin/update-libraries.sh

RUN echo 'alias arduino-build="./build.sh build"\nalias arduino-test="./build.sh test"\nalias arduino-build-test="./build.sh all"' >> /home/vscode/.bashrc \
      && echo 'set -gx PATH /usr/local/bin $PATH' >> /home/vscode/.config/fish/config.fish

RUN if [ ! -f /home/vscode/.ssh/id_rsa ]; then \
      ssh-keygen -t rsa -b 4096 -N "" -C "devcontainer@local" -f /home/vscode/.ssh/id_rsa && \
      chmod 600 /home/vscode/.ssh/id_rsa && \
      chmod 644 /home/vscode/.ssh/id_rsa.pub && \
      chown vscode:vscode /home/vscode/.ssh/id_rsa /home/vscode/.ssh/id_rsa.pub; \
      fi

RUN echo '#!/bin/sh\nchmod 666 /var/run/docker.sock\nnewgrp docker\nexec "$@"' > /usr/local/bin/docker-entrypoint.sh \
      && chmod +x /usr/local/bin/docker-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["sleep", "infinity"]
