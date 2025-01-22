#ifndef DallasTemperature_h
#define DallasTemperature_h

#define DALLASTEMPLIBVERSION "4.0.4"

// Configuration
#ifndef REQUIRESNEW
#define REQUIRESNEW false
#endif

#ifndef REQUIRESALARMS
#define REQUIRESALARMS true
#endif

// Includes
#include <inttypes.h>
#include <Arduino.h>

#ifdef __STM32F1__
#include <OneWireSTM.h>
#else
#include <OneWire.h>
#endif

// Constants for device models
#define DS18S20MODEL 0x10  // also DS1820
#define DS18B20MODEL 0x28  // also MAX31820
#define DS1822MODEL  0x22
#define DS1825MODEL  0x3B  // also MAX31850
#define DS28EA00MODEL 0x42

// Error Codes
#define DEVICE_DISCONNECTED_C -127
#define DEVICE_DISCONNECTED_F -196.6
#define DEVICE_DISCONNECTED_RAW -7040

#define DEVICE_FAULT_OPEN_C -254
#define DEVICE_FAULT_OPEN_F -425.199982
#define DEVICE_FAULT_OPEN_RAW -32512

#define DEVICE_FAULT_SHORTGND_C -253
#define DEVICE_FAULT_SHORTGND_F -423.399994
#define DEVICE_FAULT_SHORTGND_RAW -32384

#define DEVICE_FAULT_SHORTVDD_C -252
#define DEVICE_FAULT_SHORTVDD_F -421.599976
#define DEVICE_FAULT_SHORTVDD_RAW -32256

// Configuration Constants
#define MAX_CONVERSION_TIMEOUT 750
#define MAX_INITIALIZATION_RETRIES 3
#define INITIALIZATION_DELAY_MS 50

typedef uint8_t DeviceAddress[8];

class DallasTemperature {
public:
    struct request_t {
        bool result;
        unsigned long timestamp;
        operator bool() { return result; }
    };
    
    // Constructors
    DallasTemperature();
    DallasTemperature(OneWire*);
    DallasTemperature(OneWire*, uint8_t);

    // Setup & Configuration
    void setOneWire(OneWire*);
    void setPullupPin(uint8_t);
    void begin(void);
    bool verifyDeviceCount(void);

    // Device Information
    uint8_t getDeviceCount(void);
    uint8_t getDS18Count(void);
    bool validAddress(const uint8_t*);
    bool validFamily(const uint8_t* deviceAddress);
    bool getAddress(uint8_t*, uint8_t);
    bool isConnected(const uint8_t*);
    bool isConnected(const uint8_t*, uint8_t*);

    // Scratchpad Operations
    bool readScratchPad(const uint8_t*, uint8_t*);
    void writeScratchPad(const uint8_t*, const uint8_t*);
    bool readPowerSupply(const uint8_t* deviceAddress = nullptr);

    // Resolution Control
    uint8_t getResolution();
    void setResolution(uint8_t);
    uint8_t getResolution(const uint8_t*);
    bool setResolution(const uint8_t*, uint8_t, bool skipGlobalBitResolutionCalculation = false);

    // Conversion Configuration
    void setWaitForConversion(bool);
    bool getWaitForConversion(void);
    void setCheckForConversion(bool);
    bool getCheckForConversion(void);

    // Temperature Operations
    request_t requestTemperatures(void);
    request_t requestTemperaturesByAddress(const uint8_t*);
    request_t requestTemperaturesByIndex(uint8_t);
    int32_t getTemp(const uint8_t*, byte retryCount = 0);
    float getTempC(const uint8_t*, byte retryCount = 0);
    float getTempF(const uint8_t*);
    float getTempCByIndex(uint8_t);
    float getTempFByIndex(uint8_t);

    // Conversion Status
    bool isParasitePowerMode(void);
    bool isConversionComplete(void);
    static uint16_t millisToWaitForConversion(uint8_t);
    uint16_t millisToWaitForConversion();

    // EEPROM Operations
    bool saveScratchPadByIndex(uint8_t);
    bool saveScratchPad(const uint8_t* = nullptr);
    bool recallScratchPadByIndex(uint8_t);
    bool recallScratchPad(const uint8_t* = nullptr);
    void setAutoSaveScratchPad(bool);
    bool getAutoSaveScratchPad(void);

#if REQUIRESALARMS
    typedef void AlarmHandler(const uint8_t*);
    void setHighAlarmTemp(const uint8_t*, int8_t);
    void setLowAlarmTemp(const uint8_t*, int8_t);
    int8_t getHighAlarmTemp(const uint8_t*);
    int8_t getLowAlarmTemp(const uint8_t*);
    void resetAlarmSearch(void);
    bool alarmSearch(uint8_t*);
    bool hasAlarm(const uint8_t*);
    bool hasAlarm(void);
    void processAlarms(void);
    void setAlarmHandler(const AlarmHandler*);
    bool hasAlarmHandler();
#endif

    // User Data Operations
    void setUserData(const uint8_t*, int16_t);
    void setUserDataByIndex(uint8_t, int16_t);
    int16_t getUserData(const uint8_t*);
    int16_t getUserDataByIndex(uint8_t);

    // Temperature Conversion Utilities
    static float toFahrenheit(float);
    static float toCelsius(float);
    static float rawToCelsius(int32_t);
    static int16_t celsiusToRaw(float);
    static float rawToFahrenheit(int32_t);

#if REQUIRESNEW
    void* operator new(unsigned int);
    void operator delete(void*);
#endif

    // Conversion Completion Methods
    void blockTillConversionComplete(uint8_t);
    void blockTillConversionComplete(uint8_t, unsigned long);
    void blockTillConversionComplete(uint8_t, request_t);

private:
    typedef uint8_t ScratchPad[9];

    // Internal State
    bool parasite;
    bool useExternalPullup;
    uint8_t pullupPin;
    uint8_t bitResolution;
    bool waitForConversion;
    bool checkForConversion;
    bool autoSaveScratchPad;
    uint8_t devices;
    uint8_t ds18Count;
    OneWire* _wire;

    // Internal Methods
    int32_t calculateTemperature(const uint8_t*, uint8_t*);
    bool isAllZeros(const uint8_t* const scratchPad, const size_t length = 9);
    void activateExternalPullup(void);
    void deactivateExternalPullup(void);

#if REQUIRESALARMS
    uint8_t alarmSearchAddress[8];
    int8_t alarmSearchJunction;
    uint8_t alarmSearchExhausted;
    AlarmHandler* _AlarmHandler;
#endif
};

#endif // DallasTemperature_h