#pragma once

#include <iostream>
#include <fstream>
#include <string.h>
#include <algorithm>
#include <string>
#include "json/json.hpp"
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h> 
#endif
#include "logic_maker_type.h"

const std::string config_file_name = "plugin_maker.json";

using json = nlohmann::json;

const std::string template_h_file = "./template/template_Command.h";
const std::string template_cpp_file = "./template/template_Command.cpp";
const std::string template_logic_file = "./template/template_logic.cpp";
const std::string temlpate_cmake_file = "./template/CMakeLists.txt";

class Cread_logic_json_info
{
public:
    Cread_logic_json_info() = default;
    bool read_json_file(std::string file_name = config_file_name);

    bool make_project_path();
    bool make_logic_class_file();
    bool make_command_h_file();
    bool make_command_cpp_file();
    bool make_logic_plugin_cpp();
    bool make_do_message_h_file();
    bool make_do_message_cpp_file();
    bool make_Cmake_file();

private:
    std::string& replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value);
    bool create_logic_file(FILE*& stream, const std::string& file_name);
    bool read_template_file(const std::string& file_name, std::string& file_content);

public:
    CCommandList command_list_;
    CLogicClassList logic_class_list_;
    CPlugin_Project_Info plugin_project_info_;
    CAsynMessageList asyn_message_list_;
};

