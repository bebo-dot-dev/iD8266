# iD8266 - General Purpose firmware for ESP8266

Some background information here: https://jjssoftware.github.io/id8266-aka-skynet/
Some videos here: https://www.youtube.com/channel/UCa_exk34O_W2tTnGeHXw5Og

![IP](https://github.com/jjssoftware/iD8266/raw/master/demo/login.png)![SP](https://github.com/jjssoftware/iD8266/raw/master/demo/settings.png)

## Features
* Everything is or should be configurable via the web UI
* Web application authentication with 2 factor TOTP
* Web application session management
* Complexify for strong passwords
* Web pages built with efficient HTML/CSS/JS
* Web pages served from SPIFFS
* Web pages built with jquery and bootstrap for a nice user experience
* Web socket server support
* Supports AP/STA/AP+STA modes
* Supports DHCP / static IP addressing
* NTP client support built in
* MQTT pubsub client support built in
* Onboard GPIO support
* Worcester Bosch Digistat MK2 433MHz device support
* DHT22 support
* Nexactrl / HomeEasy 433MHz device support
* Deep sleep support
* gulpfile.js included for inlining and minification of HTML/CSS/JS
* OTA update support

## Getting Started
* You can find compiled binaries at GitHub's [release](https://github.com/jjssoftware/iD8266/releases) facility

### What You Will Need
### Hardware
* An ESP8266 module or development board like **WeMos D1 mini** or **NodeMcu 1.0** with at least **32Mbit Flash (equals to 4MBytes)** (ESP32 does not supported for now)

### Known Issues
* Nothing for now.

## License

MIT License
Copyright (c) 2018 Joe