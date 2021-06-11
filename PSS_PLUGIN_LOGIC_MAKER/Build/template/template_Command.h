#pragma once
#include <iostream>

#include "IFrameObject.h"
[include file]

//define command id
[command id define]

class C[command class name]
{
public:
	void Init(ISessionService* session_service);

	[command logic function define]

private:
	ISessionService* session_service_ = nullptr;
	[do message logic]
};

