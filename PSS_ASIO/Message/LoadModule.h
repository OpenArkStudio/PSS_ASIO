﻿#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "LoadLibrary.hpp"
#include "singleton.h"
#include "TimeStamp.hpp"
#include "FrameObject.hpp"
#include "tms.hpp"
#include "ISessionService.h"

//定义插件函数指针入口
using load_module_function_ptr = int(*)(IFrame_Object*, string module_param);
using unload_module_function_ptr = void(*)(void);
using do_message_function_ptr = int(*)(const CMessage_Source&, std::shared_ptr<CMessage_Packet>, std::shared_ptr<CMessage_Packet>);
using get_module_state_function_ptr = bool(*)(uint32&);
using set_output_function_ptr = void(*)(shared_ptr<spdlog::logger>);
using module_run_finction_ptr = int(*)(std::shared_ptr<CMessage_Packet>, std::shared_ptr<CMessage_Packet>);

//定义map版本的数据结构
using command_to_module_function = unordered_map<uint16, Logic_message_dispose_fn>;

//定义模块间调用的信息
using plugin_name_to_module_run = unordered_map<std::string, module_run_finction_ptr>;

class _ModuleInfo
{
public:
    string           module_file_name_;     //模块文件名称
    string           module_file_path_;     //模块路径
    string           module_param_;         //模块启动参数
    PSS_Time_Point   load_module_time_ = CTimeStamp::Get_Time_Stamp(); //模块创建时间
    Pss_Library_Handler hModule_                     = nullptr;
    load_module_function_ptr load_module_            = nullptr;
    unload_module_function_ptr unload_module_        = nullptr;
    do_message_function_ptr do_message_              = nullptr;
    get_module_state_function_ptr get_module_state_  = nullptr;
    set_output_function_ptr set_output_              = nullptr;
    module_run_finction_ptr module_run_finction_ptr_ = nullptr;

    vector<uint16> command_id_list_;

    _ModuleInfo() = default;
};

class CLoadModule
{
public:
    CLoadModule(void) = default;

    void Close();

    void set_session_service(ISessionService* session_service);

    bool load_plugin_module(const string& module_file_path, const string& module_file_name, const string& module_param);
    bool unload_plugin_module(const string& module_file_name, bool is_delete);

    int  get_module_count() const;
    shared_ptr<_ModuleInfo> find_module_info(const char* pModuleName);

    //插件接口提供相关功能
    bool get_module_exist(const char* pModuleName);
    string get_module_param(const char* pModuleName);
    string get_module_file_path(const char* pModuleName);
    void get_all_module_name(vector<string>& vecModeInfo);

    //插件命令处理异步相关消息
    command_to_module_function& get_module_function_list();

    command_to_module_function& get_session_function_list();

    int plugin_in_name_to_module_run(const std::string& module_name, std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet);

private:
    bool load_module_info(shared_ptr<_ModuleInfo> module_info) const;    //开始加载模块的接口和数据

    void delete_module_name_list(const string& module_name);

    void add_command_to_module_function(const CLogic_Command_Info& logic_command_info, std::shared_ptr<_ModuleInfo> module_info);

    void add_command_to_session_function(const CLogic_Command_Info& logic_command_info, std::shared_ptr<_ModuleInfo> module_info);

    using hashmapModuleList = unordered_map<string, shared_ptr<_ModuleInfo>>;
    hashmapModuleList module_list_;
    vector<string>    module_name_list_;               //当前插件名称列表

    command_to_module_function command_to_module_function_;     //异步线程执行列表
    command_to_module_function command_to_session_function_;    //同步线程执行列表
    plugin_name_to_module_run plugin_name_to_module_run_;
    ISessionService* session_service_ = nullptr;
};
