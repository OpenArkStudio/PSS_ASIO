#include "ConnectCounter.h"

uint32 CConnectCounter::CreateCounter()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return ++count_index;
}

