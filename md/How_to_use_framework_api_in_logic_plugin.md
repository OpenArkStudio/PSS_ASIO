Overview
========

You can use the API provided by the framework to simplify your logic plug-in development.

Table of Contents
=================
 - [How to register a message event](#How-to-egister-a-message-event)
 - [How to connect other server io](#How-to-connect-other-server-io)
 - [How to recv or send other server io](#How-to-recv-or-send-other-server-io)
 - [How to synchronize send message](#How-to-synchronize-send-message)
 - [How to create a framework timed message](#How-to-create-a-framework-timed-message)
 - [How to transparently transmit data from one IO to another IO](#How-to-transparently-transmit-data-from-one-IO-to-another-IO)
 - [How to delete io to io](#How-to-delete-io-to-io)
 - [How to synchronously send inter-plugin message](#How-to-synchronously-send-inter-plugin-message)
 - [How to make api to other plgin module](#How-to-make-api-to-other-plgin-module)

How to register a message event
===============================
(1) Create a registration message in the function loaded by the plugin(at load_module function)  
or example: I need to register a message subscription event for the establishment of a TCP link to the framework.  
```c++
int load_module(IFrame_Object* frame_object, string module_param)
{
    base_command = std::make_shared<CBaseCommand>();

    frame_object->Regedit_command(LOGIC_COMMAND_CONNECT);
    ...
}
```    
(2) Handle the news corresponding to one's own subscription.  
```c++
int do_module_message(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    MESSAGE_FUNCTION_BEGIN(recv_packet->command_id_);
    MESSAGE_FUNCTION(LOGIC_COMMAND_CONNECT, base_command->logic_connect, source, recv_packet, send_packet);
    MESSAGE_FUNCTION_END;
}
```    
(3) make class CBaseCommand.  
make CBaseCommand.h file.  
```c++
class CBaseCommand
{
public:
    void logic_connect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
```  
make CBaseCommand.cpp file.  
```c++
void CBaseCommand::logic_connect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    //do your logic
    ...
}
```  
| name | type | Description |  
| ------ | ------ | ------ |  
| source | object | IO type info |  
| recv_packet | message | recv message |  
| send_packet | message | send message |  

How to connect other server io
==============================
For example, I want to establish a tcp or udp link to a remote server.  
```c++
void CBaseCommand::logic_connect_tcp()
{
    //测试服务器间链接，链接本地10003端口
    CConnect_IO_Info io_info;
    EM_CONNECT_IO_TYPE io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;

    io_info.send_size = 1024;
    io_info.recv_size = 1024;
    io_info.server_ip = "127.0.0.1";
    io_info.server_port = 10003;
    io_info.client_ip = "127.0.0.1";
    io_info.client_port = 10091;
    io_info.server_id = 1001;
    io_info.packet_parse_id = 1;

    session_service_->connect_io_server(io_info, io_type);
}

void CBaseCommand::logic_connect_udp()
{
    //测试服务器间链接，链接本地10003端口
    CConnect_IO_Info io_info;
    EM_CONNECT_IO_TYPE io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_UDP;

    io_info.send_size = 1024;
    io_info.recv_size = 1024;
    io_info.server_ip = "127.0.0.1";
    io_info.server_port = 10005;
    io_info.server_id = 1002;
    io_info.packet_parse_id = 1;

    session_service_->connect_io_server(io_info, io_type);
}
```  

How to recv or send other server io
===================================
recv message:  
Please refer to [How to register a message event](#How_to_egister_a_message_event)  
send message:(asynchronous send)  
```c++
void CBaseCommand::logic_test_asyn(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    auto send_asyn_packet = std::make_shared<CMessage_Packet>();
    send_asyn_packet->buffer_.append(recv_packet->buffer_.c_str(), recv_packet->buffer_.size());

    session_service_->send_io_message(source.connect_id_, send_asyn_packet);
}
```  

How to synchronize send message
===============================
```c++
void CBaseCommand::logic_test_sync(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    send_packet->buffer_.append(recv_packet->buffer_.c_str(), recv_packet->buffer_.size());
}
```  

How to create a framework timed message
=======================================
Sometimes, we need to send a message to our logic module regularly to let the logic module perform a certain task regularly.  
(1) You must create a thread as the basis of message delivery in your frame when the plugin is loaded. After this thread is created, it will provide services for your in-frame delivery.  
NOTE: This thread can be shared in multiple logic plug-ins, as long as it is created once in one plug-in.  
```c++
void CBaseCommand::Init(ISessionService* session_service)
{
    session_service_->create_frame_work_thread(plugin_test_logic_thread_id);
}
```
(2) Create a message to be delivered.  
For example, you want the frame to receive this message after 1 second.  
If it is executed immediately, there is no need to set "delay_timer.delay_seconds_".  
"delay_timer.timer_id_" It must be globally unique. The meaning of this parameter is that you can cancel the specified timed execution message at any time.  
```c++
void CBaseCommand::Init(ISessionService* session_service)
{
    session_service_->create_frame_work_thread(plugin_test_logic_thread_id);

    auto send_message = std::make_shared<CMessage_Packet>();
    CFrame_Message_Delay delay_timer;

    delay_timer.delay_seconds_ = std::chrono::seconds(1);
    delay_timer.timer_id_ = 1001;

    send_message->command_id_ = COMMAND_TEST_FRAME;
    send_message->buffer_ = "freeeyes";
}
```

(3) Send a specified message to the framework.  
```c++
void CBaseCommand::Init(ISessionService* session_service)
{
    session_service_->create_frame_work_thread(plugin_test_logic_thread_id);

    auto send_message = std::make_shared<CMessage_Packet>();
    CFrame_Message_Delay delay_timer;

    delay_timer.delay_seconds_ = std::chrono::seconds(1);
    delay_timer.timer_id_ = 1001;

    send_message->command_id_ = COMMAND_TEST_FRAME;
    send_message->buffer_ = "freeeyes";

    session_service_->run_work_thread_logic(plugin_test_logic_thread_id, delay_timer, [this]() {
        PSS_LOGGER_DEBUG("[run_work_thread_logic]arrived.");
    });
}
```

(4) Of course, you can cancel a message that has been scheduled to be executed.  
For example, cancel a "1001"("delay_timer.timer_id_") task message.  
```c++
void CBaseCommand::Init(ISessionService* session_service)
{
    session_service_->delete_frame_message_timer(1001);
}
```  

How to transparently transmit data from one IO to another IO
============================================================
It is equivalent to bridging two IOs together, the data is directly transmitted transparently, and data messages are no longer delivered to the logic plug-in.  
For example, the local 10010 port data is delivered to the 10003 port.  
```c++
void CBaseCommand::Init(ISessionService* session_service)
{
    _ClientIPInfo from_io;
    from_io.m_strClientIP = "127.0.0.1";
    from_io.m_u2Port = 10010;

    _ClientIPInfo to_io;
    to_io.m_strClientIP = "127.0.0.1";
    to_io.m_u2Port = 10003;

    session_service_->add_session_io_mapping(from_io,
        EM_CONNECT_IO_TYPE::CONNECT_IO_TCP,
        to_io,
        EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_TCP);
}
```  

How to delete io to io
======================
```c++
void CBaseCommand::Init(ISessionService* session_service)
{
    _ClientIPInfo from_io;
    from_io.m_strClientIP = "127.0.0.1";
    from_io.m_u2Port = 10010;

    _ClientIPInfo to_io;
    to_io.m_strClientIP = "127.0.0.1";
    to_io.m_u2Port = 10003;

    session_service_->delete_session_io_mapping(from_io, to_io);
}
```  

How to synchronously send inter-plugin message
==============================================
You can synchronously call the interfaces of other logic plug-ins during the running of the plug-in,  
but be aware that in multi-threading, you need to add data locks to your plug-in logic.  
You must implement the module_run method of the logic module.  

```c++
int module_run(std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet)
{
    // do your job.
    PSS_LOGGER_DEBUG("[module_run]command_id_={0}.\n", send_packet->command_id_);
    return_packet->buffer_ = send_packet->buffer_;
    return_packet->command_id_ = send_packet->command_id_;
    return 0;
}
```

```c++
void CBaseCommand::logic_connect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
#if PSS_PLATFORM == PLATFORM_WIN
    std::string module_name = "Test_Logic.dll";
#else
    std::string module_name = "libTest_Logic.so";
#endif
    auto module_send_packet = std::make_shared<CMessage_Packet>();
    auto module_return_packet = std::make_shared<CMessage_Packet>();
    module_send_packet->command_id_ = 0x5000;
    session_service_->module_run(module_name, module_send_packet, module_return_packet);
}
```

How to make api to other plgin module
=====================================
You can register an api to the framework in the plug-in so that other plug-ins can use it directly. And get the result.  
```c++
    //register api with the framework
    auto test_api = std::bind(&CBaseCommand::do_logic_api, base_command.get(), std::placeholders::_1);
    session_service->add_plugin_api("test_logic", test_api);
```

other plugin use:  
```c++
    //use api with the framework
    std::string return_data = session_service->do_plugin_api("test_logic", "hello free eyes");
```
