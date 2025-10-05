#pragma once

#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP __DATE__ " " __TIME__
#endif

constexpr char FIRMWARE_BUILD_INFO[] = BUILD_TIMESTAMP;
