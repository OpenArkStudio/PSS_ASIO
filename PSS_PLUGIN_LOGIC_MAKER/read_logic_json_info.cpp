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

            logic_class.message_type_ = class_parse["message type"];
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
        for (const auto& command_parse : json_config["message map"])
        {
            CCommandInfo command_info;

            command_info.command_macro_ = command_parse["command macro"];
            command_info.command_id_ = command_parse["command id"];
            command_info.command_function_ = command_parse["command function"];
            command_info.message_in_ = command_parse["message in"];
            command_info.message_out_ = command_parse["message out"];

            command_list_.command_list_.emplace_back(command_info);
        }

        //读取异步发送指令
        for (const auto& asyn_message : json_config["asynchronous send interface"])
        {
            asyn_message_list_.asyn_message_list_.emplace_back(asyn_message["message out"]);
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
    ret = mkdir(project_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
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
    if (false == create_logic_file(stream, logic_class_file))
    {
        return false;
    }

    std::string line;
    line = "#pragma once\n\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "#include <string>\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "#include \"ReadBuffer.hpp\"\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "#include \"WriteBuffer.hpp\"\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "#include \"define.h\"\n\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);

    for (const auto& class_info : logic_class_list_.class_list_)
    {
        if (class_info.message_type_ != "Message Input" && class_info.message_type_ != "Message Output")
        {
            continue;
        }

        line = "class " + class_info.class_name_ + "\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        line = "{\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        if (class_info.message_type_ == "Message Input")
        {
            //将输入转换
            line = "public:\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\tvoid read_message(const std::string* buffer)\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\t{\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\t\tCReadBuffer read_buffer_;\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\t\tread_buffer_.append(buffer);\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);

            for (const auto& param_info : class_info.param_list_)
            {
                if (param_info.param_size_ == 0)
                {
                    line = "\t\tread_buffer_ >> " + param_info.param_name_ + ";\n";
                    fwrite(line.c_str(), line.length(), sizeof(char), stream);
                }
                else
                {
                    line = "\t\tread_buffer_ .read_data_to_string(" + param_info.param_name_ + ", " + std::to_string(param_info.param_size_) + ");\n";
                    fwrite(line.c_str(), line.length(), sizeof(char), stream);
                }
            }

            line = "\t}\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
        }
        else
        {
            //将输出转换
            line = "public:\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\tvoid write_message(std::string* buffer)\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\t{\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\t\tCWriteBuffer write_buffer_;\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\t\twrite_buffer_.append(buffer);\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);

            for (const auto& param_info : class_info.param_list_)
            {
                if (param_info.param_size_ == 0)
                {
                    line = "\t\twrite_buffer_ << " + param_info.param_name_ + ";\n";
                    fwrite(line.c_str(), line.length(), sizeof(char), stream);
                }
                else
                {
                    line = "\t\ttwrite_buffer_ .write_data_from_string(" + param_info.param_name_ + ");\n";
                    fwrite(line.c_str(), line.length(), sizeof(char), stream);
                }
            }

            line = "\t}\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
        }

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

bool Cread_logic_json_info::make_command_h_file()
{
    std::string h_file_content;
    std::string project_path = plugin_project_info_.plugin_path + plugin_project_info_.plugin_project_name;
    std::string logic_file = project_path + "/" + plugin_project_info_.plugin_project_name + "_command.h";
    FILE* stream = nullptr;

    //打开文件
    if (false == create_logic_file(stream, logic_file))
    {
        return false;
    }

    //打开模板内容
    std::string template_h_content;
    read_template_file(template_h_file, template_h_content);

    //添加头文件
    h_file_content = template_h_content;
    std::string class_h_file = "#include \"" + plugin_project_info_.plugin_project_name + "_do_message.h\"";
    replace_all_distinct(h_file_content, "[include file]", class_h_file);

    //声明命令ID
    std::string command_id_define;
    for (const auto command_id_info : command_list_.command_list_)
    {
        if (command_id_info.command_id_ != "")
        {
            command_id_define += "const uint16 " + command_id_info.command_macro_ + " = " + command_id_info.command_id_ + ";\n";
        }
    }
    replace_all_distinct(h_file_content, "[command id define]", command_id_define);

    //声明类名
    replace_all_distinct(h_file_content, "[command class name]", plugin_project_info_.plugin_project_name + "_command");

    //声明处理函数
    std::string command_func_define;
    for (const auto command_id_info : command_list_.command_list_)
    {
        if (command_func_define == "")
        {
            command_func_define += "void " + command_id_info.command_function_ + "(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);\n";
        }
        else
        {
            command_func_define += "\tvoid " + command_id_info.command_function_ + "(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);\n";
        }
    }
    replace_all_distinct(h_file_content, "[command logic function define]", command_func_define);

    //声明处理类
    std::string do_message_class = "C" + plugin_project_info_.plugin_project_name + "_do_message do_logic;";
    replace_all_distinct(h_file_content, "[do message logic]", do_message_class);

    fwrite(h_file_content.c_str(), h_file_content.length(), sizeof(char), stream);
    fclose(stream);
    return true;
}

bool Cread_logic_json_info::make_command_cpp_file()
{
    std::string cpp_file_content;
    std::string project_path = plugin_project_info_.plugin_path + plugin_project_info_.plugin_project_name;
    std::string logic_file = project_path + "/" + plugin_project_info_.plugin_project_name + "_command.cpp";
    FILE* stream = nullptr;

    //打开文件
    if (false == create_logic_file(stream, logic_file))
    {
        return false;
    }

    //打开模板内容
    std::string template_cpp_content;
    read_template_file(template_cpp_file, template_cpp_content);

    cpp_file_content = template_cpp_content;

    std::string command_func_achieve = "";
    for (const auto command_id_info : command_list_.command_list_)
    {
        command_func_achieve += "void C" + plugin_project_info_.plugin_project_name + "_command" + "::" + command_id_info.command_function_ + "(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)\n";
        command_func_achieve += "{\n\t//do message logic\n";

        if (command_id_info.message_in_ != "" && command_id_info.message_out_ != "")
        {
            command_func_achieve += "\t" + command_id_info.message_in_ + " recv;\n";
            command_func_achieve += "\t" + command_id_info.message_out_ + " send;\n";
            command_func_achieve += "\trecv.read_message(&recv_packet->buffer_);\n";
            command_func_achieve += "\tdo_logic.do_message_" + command_id_info.command_macro_
                + "(source.connect_id_, recv_packet->command_id_, recv, send);\n";
            command_func_achieve += "\tsend.write_message(&send_packet->buffer_);\n";
        }
        else if (command_id_info.message_in_ != "" && command_id_info.message_out_ == "")
        {
            command_func_achieve += "\t" + command_id_info.message_in_ + " recv;\n";
            command_func_achieve += "\trecv.read_message(&recv_packet->buffer_);\n";
            command_func_achieve += "\tdo_logic.do_message_" + command_id_info.command_macro_
                + "(source.connect_id_, recv_packet->command_id_, recv);\n";
        }

        command_func_achieve += "}\n\n";
    }

    replace_all_distinct(cpp_file_content, "[class name]", "C" + plugin_project_info_.plugin_project_name + "_command");

    replace_all_distinct(cpp_file_content, "[command logic function achieve]", command_func_achieve);

    replace_all_distinct(cpp_file_content, "[command head file]", plugin_project_info_.plugin_project_name + "_command.h");

    replace_all_distinct(cpp_file_content, "[do message logic init]", "do_logic.set_frame_object(session_service);");

    fwrite(cpp_file_content.c_str(), cpp_file_content.length(), sizeof(char), stream);
    fclose(stream);
    return true;
}

bool Cread_logic_json_info::make_logic_plugin_cpp()
{
    std::string cpp_file_content;
    std::string project_path = plugin_project_info_.plugin_path + plugin_project_info_.plugin_project_name;
    std::string logic_file = project_path + "/" + plugin_project_info_.plugin_project_name + "_logic.cpp";
    FILE* stream = nullptr;

    //打开文件
    if (false == create_logic_file(stream, logic_file))
    {
        return false;
    }

    //打开模板内容
    std::string template_cpp_content;
    read_template_file(template_logic_file, template_cpp_content);

    cpp_file_content = template_cpp_content;
    replace_all_distinct(cpp_file_content, "[include command file head file]", plugin_project_info_.plugin_project_name + "_command.h");

    replace_all_distinct(cpp_file_content, "[command class name]", "C" + plugin_project_info_.plugin_project_name + "_command");

    //开始注册代码
    std::string regedit_command;
    for (const auto command_id_info : command_list_.command_list_)
    {
        if (regedit_command == "")
        {
            regedit_command = "frame_object->Regedit_command(" + command_id_info.command_macro_ + ");\n";
        }
        else
        {
            regedit_command += "\tframe_object->Regedit_command(" + command_id_info.command_macro_ + ");\n";
        }
    }
    replace_all_distinct(cpp_file_content, "[register command id]", regedit_command);

    //开始注册事件
    std::string regedit_command_function;
    for (const auto command_id_info : command_list_.command_list_)
    {
        if (regedit_command_function == "")
        {
            regedit_command_function = "MESSAGE_FUNCTION(" + command_id_info.command_macro_ + ", base_command->" + command_id_info.command_function_ + ", source, recv_packet, send_packet);\n";
        }
        else
        {
            regedit_command_function += "\tMESSAGE_FUNCTION(" + command_id_info.command_macro_ + ", base_command->" + command_id_info.command_function_ + ", source, recv_packet, send_packet);\n";
        }
    }
    replace_all_distinct(cpp_file_content, "[message map logic function]", regedit_command_function);

    fwrite(cpp_file_content.c_str(), cpp_file_content.length(), sizeof(char), stream);
    fclose(stream);
    return true;
}

bool Cread_logic_json_info::make_do_message_h_file()
{
    std::string cpp_file_content;
    std::string project_path = plugin_project_info_.plugin_path + plugin_project_info_.plugin_project_name;
    std::string logic_file = project_path + "/" + plugin_project_info_.plugin_project_name + "_do_message.h";
    FILE* stream = nullptr;

    //打开文件
    if (false == create_logic_file(stream, logic_file))
    {
        return false;
    }

    std::string line;
    line = "#pragma once\n\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "#include \"" + plugin_project_info_.plugin_project_name + "_type.hpp\"\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "#include \"IFrameObject.h\"\n\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "class C" + plugin_project_info_.plugin_project_name + "_do_message" + "\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "{\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "public:\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "\tvoid set_frame_object(ISessionService* session_service);\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);

    for (const auto& command_id_info : command_list_.command_list_)
    {
        if (command_id_info.message_in_ != "" && command_id_info.message_out_ != "")
        {
            line = "\tvoid do_message_" + command_id_info.command_macro_ 
                + "(uint32 connect_id, uint32 command_id, " + command_id_info.message_in_ + "& recv, " 
                + command_id_info.message_out_ + "& send);\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
        }
        else if (command_id_info.message_in_ != "" && command_id_info.message_out_ == "")
        {
            line = "\tvoid do_message_" + command_id_info.command_macro_
                + "(uint32 connect_id, uint32 command_id, " + command_id_info.message_in_ + "& recv);\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
        }
    }

    for (const auto& asyn_message : asyn_message_list_.asyn_message_list_)
    {
        line = "\tvoid send_message_" + asyn_message +
            + "(uint32 connect_id, " + asyn_message + "& send);\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
    }

    line = "private:\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "\tISessionService* session_service_ = nullptr;\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);

    line = "};\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    fclose(stream);
    return true;
}

bool Cread_logic_json_info::make_do_message_cpp_file()
{
    std::string cpp_file_content;
    std::string project_path = plugin_project_info_.plugin_path + plugin_project_info_.plugin_project_name;
    std::string logic_file = project_path + "/" + plugin_project_info_.plugin_project_name + "_do_message.cpp";
    FILE* stream = nullptr;

    //打开文件
    if (false == create_logic_file(stream, logic_file))
    {
        return false;
    }

    std::string line;
    line = "#include \"" + plugin_project_info_.plugin_project_name + "_do_message.h\"\n\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);

    for (const auto& command_id_info : command_list_.command_list_)
    {
        if (command_id_info.message_in_ != "" && command_id_info.message_out_ != "")
        {
            line = "void C" + plugin_project_info_.plugin_project_name + "_do_message" + "::do_message_" + command_id_info.command_macro_
                + "(uint32 connect_id, uint32 command_id, " + command_id_info.message_in_ + "& recv, "
                + command_id_info.message_out_ + "& send)\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "{\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\t//add your logic code\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "}\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);

        }
        else if (command_id_info.message_in_ != "" && command_id_info.message_out_ == "")
        {
            line = "void C" + plugin_project_info_.plugin_project_name + "_do_message" + "::do_message_" + command_id_info.command_macro_
                + "(uint32 connect_id, uint32 command_id, " + command_id_info.message_in_ + "& recv)\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "{\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "\t//add your logic code\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
            line = "}\n\n";
            fwrite(line.c_str(), line.length(), sizeof(char), stream);
        }
    }

    for (const auto& asyn_message : asyn_message_list_.asyn_message_list_)
    {
        line = "void C" + plugin_project_info_.plugin_project_name + "_do_message" + "::send_message_" + asyn_message +
            +"(uint32 connect_id, " + asyn_message + "& send)\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        line = "{\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        line = "\tauto send_asyn_packet = std::make_shared<CMessage_Packet>();\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        line = "\tsend.write_message(&send_asyn_packet->buffer_);\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        line = "\tsession_service_->send_io_message(connect_id, send_asyn_packet);\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
        line = "}\n\n";
        fwrite(line.c_str(), line.length(), sizeof(char), stream);
    }

    line = "void C" + plugin_project_info_.plugin_project_name + "_do_message" + "::set_frame_object(ISessionService* session_service)\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "{\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "\tsession_service_ = session_service;\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);
    line = "}\n\n";
    fwrite(line.c_str(), line.length(), sizeof(char), stream);

    fclose(stream);
    return true;
}

bool Cread_logic_json_info::make_Cmake_file()
{
    std::string cpp_file_content;
    std::string project_path = plugin_project_info_.plugin_path + plugin_project_info_.plugin_project_name;
    std::string logic_file = project_path + "/CMakeLists.txt";
    FILE* stream = nullptr;

    //打开文件
    if (false == create_logic_file(stream, logic_file))
    {
        return false;
    }

    std::string cmake_content;
    read_template_file(temlpate_cmake_file, cmake_content);

    replace_all_distinct(cmake_content, "[plugin name]", plugin_project_info_.plugin_project_name);

    fwrite(cmake_content.c_str(), cmake_content.length(), sizeof(char), stream);
    fclose(stream);
    return true;
}

bool Cread_logic_json_info::create_logic_file(FILE*& stream, const std::string& file_name)
{
#ifdef _WIN32
    errno_t err = fopen_s(&stream, file_name.c_str(), "w");
    if (err != 0)
    {
        std::cout << "[Cread_logic_json_info::make_logic_class_file]file=" << file_name << ",error=" << strerror(errno) << std::endl;
        return false;
    }
#else
    stream = fopen(file_name.c_str(), "w");
    if (nullptr == stream)
    {
        std::cout << "[Cread_logic_json_info::make_logic_class_file]file=" << file_name << ",error=" << strerror(errno) << std::endl;
        return false;
    }
#endif

    return true;
}

bool Cread_logic_json_info::read_template_file(const std::string& file_name, std::string& file_content)
{
    FILE* stream = nullptr;
#ifdef _WIN32
    errno_t err = fopen_s(&stream, file_name.c_str(), "r");
    if (err != 0)
    {
        std::cout << "[Cread_logic_json_info::make_logic_class_file]file=" << file_name << ",error=" << strerror(errno) << std::endl;
        return false;
    }
#else
    stream = fopen(file_name.c_str(), "r");
    if (nullptr == stream)
    {
        std::cout << "[Cread_logic_json_info::make_logic_class_file]file=" << file_name << ",error=" << strerror(errno) << std::endl;
        return false;
    }
#endif

    //得到文件长度
    fseek(stream, 0L, SEEK_END);
    size_t file_len = ftell(stream);
    rewind(stream);

    //读取全部文件内容
    char* file_buffer = new char[file_len];
    memset(file_buffer, 0, file_len);
    fread(file_buffer, 1, file_len, stream);
    file_content = file_buffer;

    delete[] file_buffer;
    fclose(stream);
    return true;
}

std::string& Cread_logic_json_info::replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value)
{
    std::string::size_type pos = 0;
    while ((pos = str.find(old_value, pos)) != std::string::npos)
    {
        str = str.replace(pos, old_value.length(), new_value);
        if (new_value.length() > 0)
        {
            pos += new_value.length();
        }
    }
    return str;

}

