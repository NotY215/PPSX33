#pragma once
// WORK: PARAM.SFO metadata reader.
#include <string>
#include <map>
namespace ppsx33::fs {
bool sfo_parse(const std::string& path, std::map<std::string,std::string>& kv);
}
