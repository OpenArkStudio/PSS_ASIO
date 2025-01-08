﻿// 这里实现模块加载
// 一步步，便可聚少为多，便能实现目标。
// add by freeeyes
// 2020-12-20

#include "LoadModule.h"

void CLoadModule::Close()
{
    //关闭当前活跃模块
    command_to_module_function_.clear();

    for_each(module_list_.begin(), module_list_.end(), [](const std::pair<string, shared_ptr<_ModuleInfo>>& iter) {
        //关闭模块接口
        iter.second->unload_module_();

        //清除模块相关索引和数据
        CLoadLibrary::PSS_dlClose(iter.second->hModule_);
        });

    module_list_.clear();
    module_name_list_.clear();
}

void CLoadModule::set_session_service(ISessionService* session_service)
{
    session_service_ = session_service;
}

void CLoadModule::add_command_to_module_function(const CLogic_Command_Info& logic_command_info, std::shared_ptr<_ModuleInfo> module_info)
{
    if (logic_command_info.type_ == ENUM_LOGIC_COMMAND_TYPE::COMMAND_TYPE_NO_FN)
    {
        command_to_module_function_[logic_command_info.command_id_] = module_info->do_message_;
    }
    else
    {
        command_to_module_function_[logic_command_info.command_id_] = logic_command_info.logic_fn_;
    }
}

void CLoadModule::add_command_to_session_function(const CLogic_Command_Info& logic_command_info, std::shared_ptr<_ModuleInfo> module_info)
{
    if (logic_command_info.type_ == ENUM_LOGIC_COMMAND_TYPE::COMMAND_TYPE_NO_FN)
    {
        command_to_session_function_[logic_command_info.command_id_] = module_info->do_message_;
    }
    else
    {
        command_to_session_function_[logic_command_info.command_id_] = logic_command_info.logic_fn_;
    }
}

bool CLoadModule::load_plugin_module(const string& module_file_path, const string& module_file_name, const string& module_param)
{
    auto module_info = std::make_shared<_ModuleInfo>();

    //记录模块参数
    module_info->module_file_name_ = module_file_name;
    module_info->module_file_path_ = module_file_path;
    module_info->module_param_ = module_param;

    //开始注册模块函数
    if (false == load_module_info(module_info))
    {
        return false;
    }

    //查找此模块是否已经被注册，有则把信息老信息清理
    auto f = module_list_.find(module_info->module_file_name_);

    if (module_list_.end() != f)
    {
        //卸载旧的插件
        PSS_LOGGER_DEBUG("[CLoadModule::LoadMoudle]module_name = {0}, Execute Function LoadModuleData is error!", module_file_name);
        return false;
    }

    //开始调用模块初始化动作
    CFrame_Object module_frame_object;
    module_frame_object.session_service_ = session_service_;
    int nRet = module_info->load_module_((IFrame_Object* )&module_frame_object, module_info->module_param_);

    if (nRet != 0)
    {
        PSS_LOGGER_DEBUG("[CLoadModule::LoadMoudle]module_name = {0}, Execute Function LoadModuleData is error!", module_file_name);
        return false;
    }

    //获得所有的注册指令(注册)
    for (const auto& command_info : module_frame_object.module_command_list_)
    {
        //这里追加判定，是同步执行消息还是异步执行消息
        if (command_info.logic_run_state_ == EM_LOGIC_RUN_STATE::LOGIC_RUN_ASYNHRONOUS)
        {
            //异步线程执行
            add_command_to_module_function(command_info, module_info);
        }
        else
        {
            //同步session内执行
            add_command_to_session_function(command_info, module_info);
        }

        //记录当前插件加载的命令信息
        module_info->command_id_list_.emplace_back(command_info.command_id_);
    }

    //添加模块间调用的映射关系
    plugin_name_to_module_run_[module_file_name] = module_info->module_run_finction_ptr_;

    //将注册成功的模块，加入到Hash数组中
    module_list_[module_file_name] = module_info;
    module_name_list_.emplace_back(module_file_name);

    PSS_LOGGER_DEBUG("[CLoadModule::LoadMoudle]Begin Load ModuleName[{0}] OK!", module_file_name);
    return true;
}

bool CLoadModule::unload_plugin_module(const string& module_file_name, bool is_delete)
{
    PSS_LOGGER_DEBUG("[CLoadModule::UnLoadModule]szResourceName={0}.", module_file_name);
    auto f = module_list_.find(module_file_name);

    if (module_list_.end() == f)
    {
        return false;
    }
    else
    {
        //清除和此关联的所有订阅
        auto module_info = f->second;

        //关闭与list的关联
        for (auto command_id : module_info->command_id_list_)
        {
            command_to_module_function_.erase(command_id);
        }

        //调用插件关闭消息
        module_info->unload_module_();

        //清除模块相关索引和数据
        CLoadLibrary::PSS_dlClose(module_info->hModule_);

        if (true == is_delete)
        {
            module_list_.erase(f);
            delete_module_name_list(module_file_name);
        }

        PSS_LOGGER_DEBUG("[CLoadModule::UnLoadModule]Close Module={0}!", module_file_name);

        return true;
    }
}

int CLoadModule::get_module_count() const
{
    return (int)module_list_.size();
}

shared_ptr<_ModuleInfo> CLoadModule::find_module_info(const char* pModuleName)
{
    auto f = module_list_.find(pModuleName);

    if (module_list_.end() != f)
    {
        return f->second;
    }
    else
    {
        return nullptr;
    }
}

bool CLoadModule::load_module_info(shared_ptr<_ModuleInfo> module_info) const
{
    string strModuleFile;

    if (nullptr == module_info)
    {
        PSS_LOGGER_ERROR("[CLoadModule::LoadModuleInfo]module_info is nullptr!");
        return false;
    }

    strModuleFile = fmt::format("{0}{1}", module_info->module_file_path_, module_info->module_file_name_);


    module_info->hModule_ = CLoadLibrary::PSS_dlopen(strModuleFile.c_str(), RTLD_NOW);

    if (nullptr == module_info->hModule_)
    {
        PSS_LOGGER_DEBUG("[CLoadModule::LoadModuleInfo]module_name = {0}, module_info->hModule is nullptr({1})!", module_info->module_file_name_, CLoadLibrary::PSS_dlerror());
        return false;
    }

    module_info->load_module_ = (load_module_function_ptr)CLoadLibrary::PSS_dlsym(module_info->hModule_, "load_module");

    if (nullptr == module_info->load_module_)
    {
        PSS_LOGGER_DEBUG("[CLoadModule::LoadModuleInfo]module_name = {0}, Function LoadMoudle is error!", module_info->module_file_name_);
        return false;
    }

    module_info->unload_module_ = (unload_module_function_ptr)CLoadLibrary::PSS_dlsym(module_info->hModule_, "unload_module");

    if (nullptr == module_info->unload_module_)
    {
        PSS_LOGGER_DEBUG("[CLoadModule::LoadModuleInfo]module_name = {0}, Function UnloadModule is error!", module_info->module_file_name_);
        return false;
    }

    module_info->do_message_ = (do_message_function_ptr)CLoadLibrary::PSS_dlsym(module_info->hModule_, "do_module_message");

    if (nullptr == module_info->do_message_)
    {
        PSS_LOGGER_DEBUG("[CLoadModule::LoadModuleInfo]module_name = {0}, Function DoModuleMessage is error({1})!", module_info->module_file_name_, errno);
        return false;
    }

    module_info->get_module_state_ = (get_module_state_function_ptr)CLoadLibrary::PSS_dlsym(module_info->hModule_, "module_state");

    if (nullptr == module_info->get_module_state_)
    {
        PSS_LOGGER_DEBUG("[CLoadModule::LoadModuleInfo]module_name = {0}, Function GetModuleState is error({1})!", module_info->module_file_name_, errno);
        return false;
    }

    module_info->set_output_ = (set_output_function_ptr)CLoadLibrary::PSS_dlsym(module_info->hModule_, "set_output");

    if (nullptr == module_info->set_output_)
    {
        PSS_LOGGER_DEBUG("[CLoadModule::LoadModuleInfo]module_name = {0}, Function set_output is error({1})!", module_info->module_file_name_, errno);
        return false;
    }

    module_info->module_run_finction_ptr_ = (module_run_finction_ptr)CLoadLibrary::PSS_dlsym(module_info->hModule_, "module_run");
    if (nullptr == module_info->set_output_)
    {
        PSS_LOGGER_DEBUG("[CLoadModule::LoadModuleInfo]module_name = {0}, Function module_run is error({1})!", module_info->module_file_name_, errno);
        return false;
    }

    //设置日志生效
    module_info->set_output_(spdlog::default_logger());

    return true;
}

void CLoadModule::delete_module_name_list(const string& module_name)
{
    //删除vector中的name
    auto iter = std::remove(module_name_list_.begin(), module_name_list_.end(), module_name);
    module_name_list_.erase(iter, module_name_list_.end());
}

command_to_module_function& CLoadModule::get_module_function_list()
{
    return command_to_module_function_;
}

command_to_module_function& CLoadModule::get_session_function_list()
{
    return command_to_session_function_;
}


int CLoadModule::plugin_in_name_to_module_run(const std::string& module_name, std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet)
{
    auto f = plugin_name_to_module_run_.find(module_name);
    if (f != plugin_name_to_module_run_.end())
    {
        //找到了，执行代码
        return f->second(send_packet, return_packet);
    }
    else
    {
        PSS_LOGGER_DEBUG("[CLoadModule::plugin_in_name_to_module_run]module name={0} is no find.", module_name);
        return -1;
    }
}

bool CLoadModule::get_module_exist(const char* pModuleName)
{
    auto f = module_list_.find(pModuleName);

    if (module_list_.end() != f)
    {
        return true;
    }
    else
    {
        return false;
    }
}

string CLoadModule::get_module_param(const char* pModuleName)
{
    auto f = module_list_.find(pModuleName);

    if (module_list_.end() != f)
    {
        return f->second->module_param_;
    }
    else
    {
        return nullptr;
    }
}

string CLoadModule::get_module_file_path(const char* module_file_name)
{
    auto f = module_list_.find(module_file_name);

    if (module_list_.end() != f)
    {
        return f->second->module_file_path_.c_str();
    }
    else
    {
        return nullptr;
    }
}

void CLoadModule::get_all_module_name(vector<string>& module_name_list)
{
    module_name_list.clear();
    module_name_list.assign(module_name_list_.begin(), module_name_list_.end());
}


