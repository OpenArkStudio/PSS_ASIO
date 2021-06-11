#include "read_logic_json_info.h"

//逻辑插件生成工具
//add by freeeyes

int main()
{
    Cread_logic_json_info read_logic_json_info;
    if (false == read_logic_json_info.read_json_file())
    {
        getchar();
        return 0;
    }

    read_logic_json_info.make_project_path();

    read_logic_json_info.make_logic_class_file();

    read_logic_json_info.make_command_h_file();

    read_logic_json_info.make_command_cpp_file();

    read_logic_json_info.make_logic_plugin_cpp();

    read_logic_json_info.make_do_message_h_file();

    read_logic_json_info.make_do_message_cpp_file();

    getchar();
    return 0;
}
