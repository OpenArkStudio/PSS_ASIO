#pragma once

//读取配置文件信息
//add by freeeyes

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

class CPlugin_Project_Info
{
public:
    std::string plugin_project_name;
    std::string plugin_path;
};

class CLogicClass_Param_Info
{
public:
    CLogicClass_Param_Info() = default;

    std::string param_name_;
    std::string param_value_;
    int param_size_ = 0;
};

class CLogicClass
{
public:
    CLogicClass() = default;

    std::string message_type_;
    std::string class_name_;
    std::vector<CLogicClass_Param_Info> param_list_;
};

class CLogicClassList
{
public:
    CLogicClassList() = default;

    std::vector<CLogicClass> class_list_;
};

class CCommandInfo
{
public:
    CCommandInfo() = default;

    std::string command_macro_;
    std::string command_id_;
    std::string command_function_;
};

class CCommandList
{
public:
    CCommandList() = default;

    std::vector<CCommandInfo> command_list_;
};

