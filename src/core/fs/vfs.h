#pragma once
// WORK: Maps dev_flash/dev_hdd0/dev_bdvd to host paths.
#include <string>
namespace ppsx33::fs {
std::string resolve_guest_path(const std::string& guest);
void set_root_overlay(const std::string& host_root);
}
