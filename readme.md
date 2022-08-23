# Waveshare EPD 1.54in ESP32 S2 Demo

Platform IO Demo project based on [Waveshares E-Paper_ESP8266_Driver_Board_Code](https://www.waveshare.com/wiki/File:E-Paper_ESP8266_Driver_Board_Code.7z).
The librarys `src` from the zip was moved into `lib/waveshare` and the code from `examples/epd1in54-demo` was moved into `src` and renamed to `main.cpp`.

The wiring on a Lolin ESP32 S2 Mini or other ESP32 S2 based board is as following:

| EPD Pin | ESP32 S2 Pin |
| ------- | ------------ |
| CS      | 12           |
| RST     | 16           |
| DC      | 3            |
| BUSY    | 5            |
| DIN     | 11           |
| CLK     | 7            |
