#pragma once
#include <iostream>

#include "IFrameObject.h"
#include "define.h"

#include <vector>

//0是不测试框架其他接口功能，1是测试。
const uint8 TEST_FRAME_WORK_FLAG = 0;

//定义处理插件的command_id
const uint16 COMMAND_TEST_SYNC = 0x2101;
const uint16 COMMAND_TEST_ASYN = 0x2102;
const uint16 COMMAND_TEST_FRAME = 0x1100;
const uint16 COMMAND_TEST_HTTP_POST = 0x1001;
const uint16 COMMAND_WEBSOCKET_SHARK_HAND = 0x1002;
const uint16 COMMAND_WEBSOCKET_DATA = 0x1003;

const uint32 plugin_test_logic_thread_id = 1001;

class CBaseCommand
{
public:
	void Init(ISessionService* session_service);

	void logic_connect_tcp();
	void logic_connect_udp();

	void logic_connect(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_disconnect(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_test_sync(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_test_asyn(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_test_frame(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_test_connect_error(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_test_listen_error(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_http_post(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_http_websocket_shark_hand(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_http_websocket_data(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);

	ISessionService* session_service_ = nullptr;
};

