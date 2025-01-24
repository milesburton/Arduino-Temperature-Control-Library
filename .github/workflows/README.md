# 📂 GitHub Workflows for Arduino Temperature Control Library

Automate testing, compilation, and validation of the Arduino Temperature Control Library across multiple platforms using GitHub Actions.

## 🛠️ Workflows Overview

### 1. 📦 Arduino CI Workflow

**Purpose:**  
Compiles the library and its examples for both AVR and ESP8266 platforms.

**Trigger:**  
Runs on every `push` and `pull_request`.

**Key Features:**
- **AVR Compilation:** Compiles all examples for the AVR platform (e.g., Arduino Uno).
- **ESP8266 Compilation:** Compiles all examples for the ESP8266 platform (e.g., NodeMCU v2).
- **Selective Compilation:** Skips ESP-specific examples (e.g., ESP-WebServer) when compiling for AVR.
- **Unit Testing:** Executes unit tests using the `arduino_ci` framework.

### 2. 🔄 Why Separate AVR and ESP Platforms?

The library supports both AVR-based boards (e.g., Arduino Uno) and ESP8266-based boards (e.g., NodeMCU). Some examples utilize ESP-specific libraries like `ESP8266WiFi.h`, which are incompatible with AVR platforms. Separating the compilation ensures:

- **AVR Compatibility:** Skips ESP-specific examples to prevent compilation errors.
- **ESP Compatibility:** Compiles all examples, including ESP-specific ones, for the ESP8266 platform.

### 3. ⚙️ Workflow Steps

The workflow follows these steps:

1. **Setup Environment:**
   - Installs dependencies (e.g., `gcc-avr`, `avr-libc`).
   - Configures the Arduino CLI and installs required cores (`arduino:avr` and `esp8266:esp8266`).

2. **Install Libraries:**
   - Installs the OneWire library.
   - Applies a custom CRC implementation.

3. **Run Unit Tests:**
   - Executes unit tests using the `arduino_ci` framework for the AVR platform.

4. **Compile Examples for AVR:**
   - Compiles all examples (excluding ESP-specific ones) for the AVR platform.

5. **Compile Examples for ESP8266:**
   - Compiles all examples (including ESP-specific ones) for the ESP8266 platform.

### 4. 📁 File Structure

Understanding the project’s file structure is crucial for effective navigation and contribution. Below is an overview of the key files and directories:

- **`Gemfile`**
  - **Description:**  
    Manages Ruby dependencies required for the project. It ensures that the correct versions of gems (like `arduino_ci`) are used.
  - **Usage:**  
    Run `bundle install` to install the necessary gems.

- **`.arduino-ci.yml`**
  - **Description:**  
    Configuration file for the `arduino_ci` tool. It defines how the Arduino CI should run tests and compile sketches.
  - **Key Configurations:**
    - Specifies which boards to target.
    - Defines libraries and dependencies needed for testing.
    - Sets up compilation and testing parameters.

- **`.arduino_ci/`**
  - **Description:**  
    Contains supporting files and configurations for the `arduino_ci.rb` tool.
  - **Contents:**
    - **`config.rb`:**  
      Custom configuration settings for the Arduino CI.
    - **`helpers.rb`:**  
      Helper methods and utilities used by the CI scripts.
    - **Other supporting scripts and assets.**

- **`arduino-ci.yml`**
  - **Description:**  
    GitHub Actions workflow file that defines the CI pipeline for the project.
  - **Key Sections:**
    - **Jobs:**  
      Defines the sequence of steps for setting up the environment, installing dependencies, running tests, and compiling examples.
    - **Triggers:**  
      Specifies when the workflow should run (e.g., on push or pull request).

- **`examples/`**
  - **Description:**  
    Contains example sketches demonstrating how to use the Arduino Temperature Control Library.
  - **Structure:**
    - **`ESP-WebServer/`**  
      ESP-specific examples that utilize libraries like `ESP8266WiFi.h`.

- **`LICENSE`**
  - **Description:**  
    Contains the MIT License under which the project is released.

- **Other Files and Directories:**
  - **`.github/`**
    - Contains GitHub-specific configurations, issues templates, and additional workflows.
  - **`src/`**
    - Contains the source code of the Arduino Temperature Control Library.

### 5. 🔧 Workflow Configuration

The workflow is defined in the `arduino-ci.yml` file. Key configurations include:

- **Cores Installed:**
  ```yaml
  arduino-cli core install arduino:avr
  arduino-cli core install esp8266:esp8266
  ```

- **Skipping ESP-Specific Examples:**
  ```yaml
  export ARDUINO_CI_SKIP_EXAMPLES="ESP-WebServer"
  ```

- **Compiling for AVR and ESP Platforms:**
  ```yaml
  arduino-cli compile --fqbn arduino:avr:uno "$sketch"
  arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 "$sketch"
  ```

### 6. 🤝 Contributing

If you’re contributing to the workflows, please ensure that:

- **Compatibility:** New examples are compatible with both AVR and ESP platforms (if applicable).
- **Organization:** ESP-specific examples are placed in a clearly labeled directory (e.g., `examples/ESP-WebServer`).
- **Testing:** Unit tests are added or updated as needed.

### 7. 🐞 Troubleshooting

If the workflow fails:

1. **Check Logs:** Navigate to the Actions tab in GitHub for detailed logs.
2. **Local Replication:** Try to replicate the issue locally using the dev container.
3. **Dependencies:** Ensure all dependencies are installed correctly.

## 📄 License

This workflow configuration and scripts are released under the [MIT License](LICENSE).
