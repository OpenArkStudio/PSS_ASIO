// Define the logic plug-in call interface

#include <iostream>

#include "IFrameObject.h"
#include "define.h"

#include "[include command file head file]"

#if PSS_PLATFORM == PLATFORM_WIN
#ifdef TEST_LOGIC_EXPORTS
#define DECLDIR extern "C" _declspec(dllexport)
#else
#define DECLDIR extern "C"__declspec(dllimport)
#endif
#else
#define DECLDIR extern "C"
#endif

using namespace std;

DECLDIR int load_module(IFrame_Object* frame_object, string module_param);
DECLDIR void unload_module();
DECLDIR int do_module_message(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
DECLDIR int module_state();
DECLDIR int module_run(std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet);
DECLDIR void set_output(shared_ptr<spdlog::logger> logger);

ISessionService* session_service = nullptr;
std::shared_ptr<[command class name]> base_command = nullptr;

#define MESSAGE_FUNCTION_BEGIN(x) switch(x) {
#define MESSAGE_FUNCTION(x,y,z,h,i) case x: { y(z,h,i); break; }
#define MESSAGE_FUNCTION_END }

//Plugin loading
int load_module(IFrame_Object* frame_object, string module_param)
{
#ifdef GCOV_TEST
    //If it is a functional code coverage check, turn on this switch and let the plug-in execute all framework interface calls
    PSS_LOGGER_DEBUG("[load_module]gcov_check is set.");
#endif

    //Initialize the message processing class
    base_command = std::make_shared<[command class name]>();

    //Register the plugin
    [register command id]

    session_service = frame_object->get_session_service();

    base_command->Init(session_service);

    PSS_LOGGER_DEBUG("[load_module]({0})finish.", module_param);

    return 0;
}

//Uninstall plugin
void unload_module()
{
    PSS_LOGGER_DEBUG("[unload_module]finish.");
}

//Perform message processing
int do_module_message(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    //Plug-in message processing
    //PSS_LOGGER_DEBUG("[do_module_message]command_id={0}.", recv_packet.command_id_);

    MESSAGE_FUNCTION_BEGIN(recv_packet->command_id_);
    [message map logic function]
    MESSAGE_FUNCTION_END;

    return 0;
}

//Synchronous calls between modules
int module_run(std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet)
{
    //Add your logic processing code here
    PSS_LOGGER_DEBUG("[module_run]command_id_={0}.\n", send_packet->command_id_);
    return_packet->buffer_ = send_packet->buffer_;
    return_packet->command_id_ = send_packet->command_id_;
    return 0;
}

//Get the current plug-in status
int module_state()
{
    return 0;
}

//Set log output
void set_output(shared_ptr<spdlog::logger> logger)
{
    spdlog::set_default_logger(logger);
}

