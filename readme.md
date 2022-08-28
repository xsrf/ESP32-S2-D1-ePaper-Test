# Waveshare EPD 1.54in ESP32 S2 Demo

The wiring on a Lolin ESP32 S2 Mini or other ESP32 S2 based board is as following:

| EPD Pin | ESP32 S2 Pin |
| ------- | ------------ |
| CS      | 12           |
| RST     | 16           |
| DC      | 3            |
| BUSY    | 5            |
| DIN     | 11           |
| CLK     | 7            |

# Setup
After first compile, use `Upload filesystem image` to upload the webinterface.
# Upload pictures
Connect to the ESPs SoftAP (or connect it to your wifi by commenting out the one line in `setup()`) and open `http://192.168.4.1/` or its local IP.
Got to `/edit` to edit the filesystem / upload 4-bit bitmaps.
Use `/show?file=/muc-4bit-r.bmp` to show a file on the epaper.
# SoftAP Credentials
Standard is SSID "ePaper-xxxxxx" and passphrase "epd-pass".