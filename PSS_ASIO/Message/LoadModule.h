#pragma once

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
using do_message_function_ptr = int(*)(const CMessage_Source&, const CMessage_Packet&, CMessage_Packet&);
using get_module_state_function_ptr = bool(*)(uint32&);
using set_output_function_ptr = void(*)(shared_ptr<spdlog::logger>);

//定义map版本的数据结构
using command_to_module_function = unordered_map<uint16, do_message_function_ptr>;

class _ModuleInfo
{
public:
    string           module_file_name_;     //模块文件名称
    string           module_file_path_;     //模块路径
    string           module_param_;         //模块启动参数
    PSS_Time_Point   load_module_time_ = CTimeStamp::Get_Time_Stamp(); //模块创建时间
    Pss_Library_Handler hModule_                    = nullptr;
    load_module_function_ptr load_module_           = nullptr;
    unload_module_function_ptr unload_module_       = nullptr;
    do_message_function_ptr do_message_             = nullptr;
    get_module_state_function_ptr get_module_state_ = nullptr;
    set_output_function_ptr set_output_             = nullptr;

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

    //插件命令处理同步相关功能
    command_to_module_function& get_module_function_list();

private:
    bool load_module_info(shared_ptr<_ModuleInfo> module_info);    //开始加载模块的接口和数据

    void delete_module_name_list(const string& module_name);

    using hashmapModuleList = unordered_map<string, shared_ptr<_ModuleInfo>>;
    hashmapModuleList                  module_list_;
    vector<string>                     module_name_list_;               //当前插件名称列表

    command_to_module_function command_to_module_function_;
    ISessionService* session_service_;
};
