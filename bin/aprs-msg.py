#!/usr/bin/env python
import gps

def gpsd_query(host='localhost', port=2947):
    session = gps.gps(host=host, port=port)
    session.stream(gps.WATCH_ENABLE|gps.WATCH_NEWSTYLE)
    for report in session:
        if 'lat' in report:
            return report['lat'], report['lon']

def aprs_loc(lat, lon):
    ns = 'N' if lat >= 0 else 'S'
    ew = 'E' if lon >= 0 else 'W'
    lat, lon = abs(lat), abs(lon)
    lat_deg = int(lat)
    lat_min = ((lat % 1) * 3600) / 60
    lon_deg = int(lon)
    lon_min = ((lon % 1) * 3600) / 60
    return "!{:02d}{:05.02f}{}/{:03d}{:05.02f}{}>".format(lat_deg, lat_min, ns, lon_deg, lon_min, ew)

if __name__ == '__main__':
    lat, lon = gpsd_query()
    #lat, lon = (49.05833333,0)
    print(aprs_loc(lat, lon))
