#pragma once

#include <vector>
#include <array>
#include <string>
#include <map>
#include <string_view>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <thread>
#include <chrono>

//#define NO_IMPL { std::cerr << "NOT YET IMPLEMENTED" << std::endl; std::exit(-5); }

static std::thread cpuThread;

namespace SDL2Timing {
    void delay(std::chrono::milliseconds duration);
    uint32_t getTicks();
}