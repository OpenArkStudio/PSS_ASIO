#pragma once
#include <iostream>

#include "IFrameObject.h"
#include "define.h"
#include "PerformanceCheck.hpp"

#include <vector>

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
	void init(ISessionService* session_service);

	void close();

	void logic_connect_tcp();
	void logic_connect_udp();
	void test_io_2_io();
	void test_create_io_listen();
	void test_close_io_listen();

	void logic_connect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_disconnect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	int logic_test_sync(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	int logic_test_asyn(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_test_frame(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_test_connect_error(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_test_listen_error(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_http_post(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_http_websocket_shark_hand(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_http_websocket_data(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_work_thread_is_lock(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
	void logic_io_write_error(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);

	std::string do_logic_api(std::string api_param);

	void performace_check(const std::string name, const double time_cost_millsecond);

	ISessionService* session_service_ = nullptr;
};

