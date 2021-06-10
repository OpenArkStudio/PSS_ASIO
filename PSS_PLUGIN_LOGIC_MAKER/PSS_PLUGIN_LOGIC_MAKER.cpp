#include "read_logic_json_info.h"

//逻辑插件生成工具
//add by freeeyes

int main()
{
    Cread_logic_json_info read_logic_json_info;
    read_logic_json_info.read_json_file();

    read_logic_json_info.make_project_path();

    read_logic_json_info.make_logic_class_file();

    read_logic_json_info.make_command_h_file();

    read_logic_json_info.make_command_cpp_file();

    read_logic_json_info.make_logic_plugin_cpp();

    getchar();
    return 0;
}
