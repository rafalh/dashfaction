#pragma once

#include "../rf.h"

void ProcessPfPacket(const void* data, size_t len, const rf::NwAddr& addr, const rf::Player* player);
