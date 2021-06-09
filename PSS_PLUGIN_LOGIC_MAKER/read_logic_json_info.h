#pragma once

#include <iostream>
#include <fstream>
#include <string.h>
#include "json/json.hpp"
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h> 
#endif
#include "logic_maker_type.h"

const std::string config_file_name = "plugin_maker.json";

using json = nlohmann::json;

class Cread_logic_json_info
{
public:
    Cread_logic_json_info() = default;
    bool read_json_file(std::string file_name = config_file_name);

    bool make_project_path();

    bool make_logic_class_file();

public:
    CCommandList command_list_;
    CLogicClassList logic_class_list_;
    CPlugin_Project_Info plugin_project_info_;
};

