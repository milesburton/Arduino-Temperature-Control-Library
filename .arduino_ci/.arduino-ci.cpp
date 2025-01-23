#define ARDUINO_CI 1

// Mock OneWire GPIO functions
uint8_t digitalPinToBitMask(uint8_t pin) { return 1 << (pin % 8); }
void* digitalPinToPort(uint8_t pin) { static uint8_t dummy; return &dummy; }
void* portModeRegister(void* port) { return port; }