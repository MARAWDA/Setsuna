#line 1 "C:\\Users\\wisht\\New folder\\Setsuna\\esp32_marauder\\PcapHeader.h"
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace marauder {

constexpr size_t kPcapGlobalHeaderSize = 24;

void makePcapGlobalHeader(uint32_t snapshot_length,
                          uint8_t output[kPcapGlobalHeaderSize]);

}  // namespace marauder
