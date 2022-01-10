## Overview

Utilities for sending [APRS](https://en.wikipedia.org/wiki/Automatic_Packet_Reporting_System) packets with an SDR. It can be used both as Web app and standalone binary.

## Web version
The Web version is using the [HTML Geolocation API](https://developer.mozilla.org/en-US/docs/Web/API/Geolocation_API) to retrieve the device's position and [WebUSB](https://developer.mozilla.org/en-US/docs/Web/API/WebUSB_API) to talk with the SDR. It is deployed at [https://xakcop.com/aprs-sdr](http://xakcop.com/aprs-sdr) and works like this:
![](/docs/aprs-hackrf.gif)

For now it supports only [HackRF](https://greatscottgadgets.com/hackrf/) but more SDRs can be added in the future. I have been testing only with the Google Chrome browser on Linux and Android (Pixel phones). My results show that the tracker works great on desktop Chrome and not so great on mobile Chrome. Sometimes, after sending a couple of packets on mobile Chrome, the scripts hangs at `device.transferOut()`. I guess this is a bug in the mobile implementation of WebUSB but I could be wrong. Any ideas how to fix this are appreciated. Feedback for other browsers and platforms is also welcome!

### Building from source
You need the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) to build the Web version:
```
$ CXX=emcc make
```
The site should be served from the `/docs` folder.

## Desktop version
The desktop version relies on `gpsd` to retrieve the device's position:
![](/docs/aprs-sdr.png)

* GPS device - Any GPS device supported by `gpsd`. You can also use a mobile phone and stream GPS data using an app like this [one](https://play.google.com/store/apps/details?id=io.github.tiagoshibata.gpsdclient&hl=en&gl=US)
* [gpsd](https://gpsd.gitlab.io/gpsd/) - GPS service daemon
* aprs-sdr - This program which retrieves data from `gpsd`, generates APRS messages and produces complex sampled IQ files that can be transmitted with an SDR
* SDR device - Any transmit capable SDR (e.g. HackRF, USRP, LimeSDR, etc.)

### Building and usage
Simply run `make`. You can find some example scripts in the `/bin` folder.
