#include "ConnectCounter.h"

uint32 CConnectCounter::CreateCounter()
{
    return ++count_index;
}

