#pragma once
// WORK: Unencrypted PSARC unpack.
#include <string>
#include <vector>
#include <cstdint>
namespace ppsx33::fs {
bool psarc_open(const std::string& path);
bool psarc_read(const std::string& entry, std::vector<uint8_t>& out);
}
