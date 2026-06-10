#pragma once

#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <fstream>
#include <memory>
#include <ctime>
#include <unistd.h>
#include "mutex.hpp"

namespace ns_log
{
    /************显示*************/
    // 显示策略 : a.显示器打印  b.向指定文件写入
    class LogStrategy
    {
    public:
        virtual void SyncLog(const std::string &message) = 0;
    };
    // a:
    class ConsoleLogStrategy : public LogStrategy
    {
    public:
        ConsoleLogStrategy()
        {
        }
        ~ConsoleLogStrategy()
        {
        }
        void SyncLog(const std::string &message) override
        {
            MutexGuard mutexguard(_mutex);
            std::cout << message << "\r\n";
        }

    private:
        Mutex _mutex;
    };
    // b:
    class FileLogStrategy : public LogStrategy
    {
    public:
        FileLogStrategy(std::string path = "./log", std::string file = "log.txt")
            : _path(path), _file(file)
        {
            if (std::filesystem::exists(_path))
            {
                return;
            }
            try
            {
                std::filesystem::create_directories(_path);
            }
            catch (std::filesystem::filesystem_error &e)
            {
                std::cout << e.what() << std::endl;
            }
        }
        ~FileLogStrategy()
        {
        }
        void SyncLog(const std::string &message) override
        {
            MutexGuard mutexguard(_mutex);
            std::string file_name = _path + (_path.back() == '/' ? "" : "/") + _file;
            std::ofstream out(file_name, std::ios::app); // 已追加方式打开
            if (!out.is_open())
            {
                return;
            }
            out << message << "\r\n";
            out.close();
        }

    private:
        std::string _path; // 指定路径
        std::string _file; // 指定文件
        Mutex _mutex;
    };
    /***************开始构造-log消息-*****************/
    // 日志等级:
    enum class LogLevel
    {
        DEBUG,   // 调试
        INFO,    // 正常
        WARNING, // 警告
        ERROR,   // 错误
        FATAL    // 致命
    };
    std::string GetLevel(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        default: 
            return "UNKNOWN";
        }
    }

    // 时间:
    std::string GetTime()
    {
        time_t now = time(nullptr);
        tm* ltm = localtime(&now);
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%4d-%02d-%02d %02d:%02d:%02d"
                , ltm->tm_year + 1900
                , ltm->tm_mon + 1
                , ltm->tm_mday
                , ltm->tm_hour
                , ltm->tm_min
                , ltm->tm_sec);
        return buffer;
    }

    //形成日志  --  根据不同的策略选择合适的刷新方法
    class Logger
    {
    public:
        //一条log消息
        class LogMessage
        {
        public:
            LogMessage(LogLevel level, std::string src, int line, Logger& logger)
                : _curr_time(GetTime())
                , _level(GetLevel(level))
                , _pid(getpid())
                , _src_name(src)
                , _line_number(line)
                , _logger(logger)
            {
                std::stringstream ss;
                ss << "[" << _curr_time << "] " << "[" << _level << "] "
                   << "[" << _pid << "] " << "[" << _src_name << "] "
                   << "[" << _line_number << "]";
                _loginfo= ss.str(); //一条日志的左部分 
            }
            //支持右半部分重载: <<, 例如 : _loginfo("hello") << " world" << " 1" -> _loginfo("hello world 1")
            template<class T>
            LogMessage& operator << (const T data)
            {
                std::stringstream ss;
                ss << data;
                _loginfo += ss.str();
                return *this;
            }
            LogMessage& operator<<(std::ostream& (*manip)(std::ostream&))
            {
                if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
                {
                    _loginfo += '\n';
                }
                return *this;
            }
            ~LogMessage()
            {
                if (_logger._fflush_strategy)
                    _logger._fflush_strategy->SyncLog(_loginfo);
            }
        private:
            std::string _curr_time; //当前时间
            std::string _level; //日志等级
            pid_t _pid; //进程pid
            std::string _src_name; //当前文件
            int _line_number; //当前行号
            std::string _loginfo; //一条完整的信息
            Logger& _logger;
        };
    public:
        Logger()
        {
            EnableConsoleLogStrategy();
        }
        ~Logger()
        {}
        void EnableFileLogStrategy()
        {
            _fflush_strategy = std::make_unique<FileLogStrategy>();
        }
        void EnableConsoleLogStrategy()
        {
            _fflush_strategy = std::make_unique<ConsoleLogStrategy>();
        }      
        LogMessage operator()(LogLevel level, std::string src, int line)
        {
            return LogMessage(level, src, line, *this);
        }
    private:
        std::unique_ptr<LogStrategy> _fflush_strategy;
    };

    Logger g_logger;
    // 必须用 ns_log::g_logger，避免与 <cmath> 的 log() 冲突
    #define LOG(level) ns_log::g_logger(level, __FILE__, __LINE__)
    #define Enable_Console_Log_Strategy ns_log::g_logger.EnableConsoleLogStrategy()
    #define Enable_File_Log_Strategy ns_log::g_logger.EnableFileLogStrategy()
}

#endif