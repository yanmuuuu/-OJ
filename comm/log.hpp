#pragma once

#include <iostream>
#include <string>

#include "util.hpp"

namespace ns_log
{
    using namespace ns_util;

    //灾害等级 : FATAL > ERROR > WARNING > DEBUG = INFO
    enum LogLevel
    {
        INFO,
        DEBUG,
        WARNING,
        ERROR,
        FATAL,
    };

    static std::ostream &log(const std::string &level, const std::string &file_name, int line)
    {
        std::string message;

        message += '[';
        message += level;
        message += ']';

        message += '[';
        message += file_name;
        message += ']';

        message += '[';
        message += std::to_string(line);
        message += ']';

        message += '[';
        message += TimeUtil::GetTimeStamp_S();
        message += ']';

        //将字段存入std::cout缓冲区中, 最后也只需要 << std::endl 刷新缓冲区即可打印出日志
        std::cout << message;

        return std::cout;
    }

#define LOG(level) log(#level, __FILE__, __LINE__)
}