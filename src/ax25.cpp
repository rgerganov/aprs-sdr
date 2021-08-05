#include "ax25.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <ctype.h>

static std::vector<uint8_t> encode_callsign(const char *callsign)
{
    char cs[16] = {0};
    if (strlen(callsign) >= sizeof(cs)) {
        fprintf(stderr, "Invalid callsign: %s\n", callsign);
        exit(1);
    }
    for (int i = 0; i < (int)strlen(callsign); i++) {
        cs[i] = toupper(callsign[i]);
    }
    int ssid = 0;
    char *pos = strchr(cs, '-');
    if (pos) {
        ssid = atoi(pos+1);
        *pos = 0;
    }
    if (strlen(cs) > 6) {
        fprintf(stderr, "Invalid callsign: %s\n", callsign);
        exit(1);
    }
    if (ssid < 0 || ssid > 15) {
        fprintf(stderr, "Invalid ssid: %d\n", ssid);
        exit(1);
    }
    char buf[8] = {0}; // callsign(6 bytes) + ssid(1 byte) + null
    sprintf(buf, "%-6s%c", cs, '0' + ssid);

    return std::vector<uint8_t>(buf, buf+7);
}

static std::vector<uint8_t> encode_address(const char *callsign, const char *dest, char *path)
{
    auto encoded_dest = encode_callsign(dest);
    auto encoded_callsign = encode_callsign(callsign);

    std::vector<uint8_t> addr;
    addr.insert(addr.end(), encoded_dest.begin(), encoded_dest.end());
    addr.insert(addr.end(), encoded_callsign.begin(), encoded_callsign.end());
    char *digi = strtok(path, ",");
    while (digi != nullptr) {
        auto encoded_digi = encode_callsign(digi);
        addr.insert(addr.end(), encoded_digi.begin(), encoded_digi.end());
        digi = strtok(nullptr, ",");
    }
    // see 3.12. Address-Field Encoding, AX25 spec
    // each of the address bytes are shifted one bit left
    std::vector<uint8_t> result;
    for (auto b : addr) {
        result.push_back(b << 1);
    }
    // the least significant bit of the last byte of the address must be set
    result[result.size()-1] |= 0x01;
    return result;
}

static uint16_t calc_fcs(const std::vector<uint8_t> &data)
{
    uint16_t ret = 0xffff;
    for (auto b : data) {
        for (int i = 0 ; i < 8 ; i++) {
            int b1 = (b & 1) != 0;
            int b2 = (ret & 1) != 0;
            ret >>= 1;
            if (b1 != b2) {
                ret ^= 0x8408;
            }
            b >>= 1;
        }
    }
    return ~ret;
}

static std::vector<bool> bit_stuffing(const std::vector<uint8_t> &data)
{
    std::vector<bool> result;
    int count = 0;
    for (auto b : data) {
        for (int i = 0 ; i < 8 ; i++) {
            if (b & 1) {
                result.push_back(true);
                count += 1;
                if (count == 5) {
                    // stuff 0 after every 5 consecutive 1s
                    result.push_back(false);
                    count = 0;
                }
            } else {
                result.push_back(false);
                count = 0;
            }
            b >>= 1;
        }
    }
    return result;
}

std::vector<bool> nrzi(const std::vector<bool> &data)
{
    // 0 is encoded as change in tone, 1 is encoded as no change in tone
    std::vector<bool> result;
    bool current = true;
    for (auto b : data) {
        if (!b) {
            current = !current;
        }
        result.push_back(current);
    }
    return result;
}

std::vector<bool> ax25frame(const char *callsign, const char *dest, char *path, const char *info, bool debug)
{
    // addr = dest (7bytes), source (7bytes), path (0-56 bytes)
    // frame = flag (0x7e), addr, control (0x03), protocol (0xf0), info (1-256 bytes), fcs (2bytes), flag (0x7e)
    const uint8_t control = 0x03;
    const uint8_t protocol = 0xf0;

    std::vector<uint8_t> frame;
    auto addr = encode_address(callsign, dest, path);
    frame.insert(frame.end(), addr.begin(), addr.end());
    frame.push_back(control);
    frame.push_back(protocol);
    frame.insert(frame.end(), info, info+strlen(info));
    auto fcs = calc_fcs(frame);
    // FCS is sent MSB first, all other fields are sent LSB first
    frame.push_back(fcs & 0xff);
    frame.push_back((fcs >> 8) & 0xff);
    auto stuffed_frame = bit_stuffing(frame);

    std::vector<bool> result;
    // frame will be preceded with zeros (which NRZI will encode as alternating tones)
    // to assist with decoder clock sync
    for (int i = 0 ; i < 20 ; i++) {
        result.push_back(false);
    }
    const std::vector<bool> flag = {0, 1, 1, 1, 1, 1, 1, 0};
    // not sure why we need so many flags at the beginning
    for (int i = 0 ; i < 100 ; i++) {
        result.insert(result.end(), flag.begin(), flag.end());
    }
    result.insert(result.end(), stuffed_frame.begin(), stuffed_frame.end());
    result.insert(result.end(), flag.begin(), flag.end());
    if (debug) {
        fprintf(stderr, "Packet: ");
        for (auto b : result) {
            fprintf(stderr, "%d", b ? 1 : 0);
        }
        printf("\n");
    }
    return result;
}
