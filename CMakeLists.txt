# Author: diplfranzhoepfinger 
# reference:  https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html
# URL: https://github.com/milesburton/Arduino-Temperature-Control-Library
# DATE: 15.02.2023

idf_component_register(
    SRCS "DallasTemperature.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES OneWire arduino
    )
