#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include "http_parser.h"

// http协议解析
// 当前只处理post请求的数据
// add by freeeyes

class CHttpTextBuffer
{
public:
    std::string http_func_name_;     //当前执行回调函数的名称
    std::string http_request_text_;  //当前的http消息文本
    std::string http_post_text_;     //当前的post消息文本
    bool is_completed_ = false;      //是否接收完成
    int buffer_parse_pos_ = 0;       //当前解析到的字节位置
    bool is_post_length_ = false;    //是否获得了Content-Length标签
    int content_length_ = 0;         //接收post消息的长度 
    bool is_upgrade = false;         //是否是版本升级
    std::string upgrade_;            //版本升级字段
    bool is_websocket_key_ = false;  //是否有websocket_key字段
    std::string websocket_key_;      //websocket_key内容

    void clear()
    {
        buffer_parse_pos_ = 0;
        is_completed_ = false;
        http_request_text_ = "";
        http_post_text_ = "";
        http_func_name_ = "";
        is_post_length_ = false;
        content_length_ = 0;
        is_upgrade = false;
        is_websocket_key_ = false;
    }
};

class CHttpFormat
{
public:
    void init_http_setting();

    int try_parse(std::string http_text);

    std::string get_post_text();
    std::string get_post_error();
    std::string get_websocket_client_key();

    std::string get_response_text(std::string data);
    std::string get_response_websocket_text(std::string data);

    bool is_websocket();

    static int sChunkComplete(http_parser* hp);
    static int sChunkHeader(http_parser* hp);
    static int sMessageEnd(http_parser* hp);
    static int sMessageBegin(http_parser* hp);
    static int sHeadComplete(http_parser* hp);
    static int sHeadValue(http_parser* hp, const char* at, size_t length);
    static int sHeadField(http_parser* hp, const char* at, size_t length);
    static int sBodyHandle(http_parser* hp, const char* at, size_t length);
    static int sStatusHandle(http_parser* hp, const char* at, size_t length);
    static int sHeaderFieldCallback(http_parser* hp, const char* at, size_t length);
    static int sUrlCallback(http_parser* hp, const char* at, size_t length);

private:
    CHttpTextBuffer http_text_buffer_;
    http_parser http_parser_;
    http_parser_settings http_settings_;
};
