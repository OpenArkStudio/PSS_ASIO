#pragma once
#include <iostream>

#include "IFrameObject.h"
#include "define.h"

#include <vector>

//define command id
[command id define]

class CBaseCommand
{
public:
	void Init(ISessionService* session_service);

	//void logic_connect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	[command logic function define]

	ISessionService* session_service_ = nullptr;
};

