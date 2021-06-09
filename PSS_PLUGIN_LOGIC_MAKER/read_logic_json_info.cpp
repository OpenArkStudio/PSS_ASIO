#include "read_logic_json_info.h"

bool Cread_logic_json_info::read_json_file(std::string file_name)
{
    try
    {
        //读取类声明
        std::ifstream config_input(file_name);
        json json_config = json::parse(config_input);

        plugin_project_info_.plugin_project_name = json_config["plugin project name"];
        plugin_project_info_.plugin_path = json_config["plugin project path"];

        for (const auto& class_parse : json_config["plugin class"])
        {
            CLogicClass logic_class;

            logic_class.class_name_ = class_parse["class name"];
            for (auto class_parse_param : class_parse["class type"])
            {
                CLogicClass_Param_Info param_info;
                param_info.param_name_ = class_parse_param["name"];
                param_info.param_value_ = class_parse_param["type"];
                if (class_parse_param["buffer_length"] != nullptr)
                {
                    param_info.param_size_ = class_parse_param["buffer_length"];
                }

                logic_class.param_list_.emplace_back(param_info);
            }

            logic_class_list_.class_list_.emplace_back(logic_class);
        }

        //读取命令
        for (const auto& command_parse : json_config["command"])
        {
            CCommandInfo command_info;

            command_info.command_macro_ = command_parse["command macro"];
            command_info.command_id_ = command_parse["command id"];
            command_info.command_function_ = command_parse["command function"];

            command_list_.command_list_.emplace_back(command_info);
        }

        std::cout << "[Cread_logic_json_info::Init]parse json ok." << std::endl;
        return true;
    }
    catch (const json::parse_error& e)
    {
        std::cout << "[Cread_logic_json_info::Init]parse error(" << e.what() << ")" << std::endl;
        return false;
    }
}

bool Cread_logic_json_info::make_project_path()
{
    std::string project_path = plugin_project_info_.plugin_path + plugin_project_info_.plugin_project_name;
    int ret = 0;
    //遍历目录，查看路径是否存在
#ifdef _WIN32
    ret = _mkdir(project_path.c_str());
#else
    ret = mkdir(project_path.c_str());
#endif

    if (ret != 0)
    {
        int path_error = errno;
        if (path_error != 17)
        {
            std::cout << "[Cread_logic_json_info::make_project_path]file=" << project_path << ",error=" << strerror(path_error) << std::endl;
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return true;
    }
}

bool Cread_logic_json_info::make_logic_class_file()
{
    std::string project_path = plugin_project_info_.plugin_path + plugin_project_info_.plugin_project_name;
    std::string logic_class_file = project_path + "/" + plugin_project_info_.plugin_project_name + "_type.hpp";
    FILE* stream = nullptr;

    //打开文件
#ifdef _WIN32
    errno_t err = fopen_s(&stream, logic_class_file.c_str(), "w");
    if (err != 0)
    {
        std::cout << "[Cread_logic_json_info::make_logic_class_file]file=" << logic_class_file << ",error=" << strerror(errno) << std::endl;
        return false;
    }
#else
    stream = fopen(logic_class_file.c_str(), "w");
    if (nullptr == stream)
    {
        std::cout << "[Cread_logic_json_info::make_logic_class_file]file=" << logic_class_file << ",error=" << strerror(errno) << std::endl;
        return false;
    }
#endif

    std::string line;
    line = "#pragma once\n\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "#include <string>\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "#include <define.h>\n\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);

    for (const auto& class_info : logic_class_list_.class_list_)
    {
        line = "class " + class_info.class_name_ + "\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        line = "{\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        line = "public:\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        for (const auto& param_info : class_info.param_list_)
        {
            line = "\t" + param_info.param_value_ + " " + param_info.param_name_ + ";\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
        }
        line = "};\n\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
    }

    fclose(stream);
    return true;
}
