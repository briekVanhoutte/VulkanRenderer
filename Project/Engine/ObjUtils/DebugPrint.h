#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

inline std::string ptrStr(const void* p) {
    std::ostringstream ss;
    ss << "0x" << std::hex << (uintptr_t)p;
    return ss.str();
}