#ifndef __AX25_H__
#define __AX25_H__
#include <vector>

std::vector<bool> ax25frame(const char *callsign, const char *dest, char *path, const char *info, bool debug);
std::vector<bool> nrzi(const std::vector<bool> &data);

#endif
