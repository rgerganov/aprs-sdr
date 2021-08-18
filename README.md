APRS tracker with transmit capable SDRs

![](/doc/aprs-sdr.png)

* GPS device - Any GPS device supported by `gpsd`. You can also use a mobile phone and stream GPS data using an app like this [one](https://play.google.com/store/apps/details?id=io.github.tiagoshibata.gpsdclient&hl=en&gl=US)
* [gpsd](https://gpsd.gitlab.io/gpsd/) - GPS service daemon
* aprs-sdr - This program which retrieves data from `gpsd`, generates APRS messages and produces complex sampled IQ files that can be transmitted with an SDR
* SDR device - Any transmit capable SDR (e.g. HackRF, USRP, LimeSDR, etc.)

