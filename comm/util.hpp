#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <atomic>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "boost/algorithm/string.hpp"

namespace ns_util
{
    static const std::string temp = "./temp/"; 

    class TimeUtil
    {
    public:
        //秒级别时间戳
        static const std::string GetTimeStamp_S()
        {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return std::to_string(tv.tv_sec);
        }

        //毫秒级别时间戳
        static const std::string GetTimeStamp_MS()
        {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return std::to_string((tv.tv_sec * 1000 + tv.tv_usec / 1000));
        }       
    };

    class PathUtil
    {
    public:
        static std::string AddSuffix(const std::string& file_name, const std::string& suffix)
        {
            std::string file = temp;
            file += file_name;
            file += suffix;
            return file;
        }

        static std::string Src(const std::string& file_name)
        {
            return AddSuffix(file_name, ".cc");
        }

        static std::string Exe(const std::string& file_name)
        {
            return AddSuffix(file_name, ".exe");
        }

        static std::string Compile_err(const std::string& file_name)
        {
            return AddSuffix(file_name, ".compile_err");
        }

        static std::string Stdin(const std::string& file_name)
        {
            return AddSuffix(file_name, ".stdin");
        }

        static std::string Stdout(const std::string& file_name)
        {
            return AddSuffix(file_name, ".stdout");
        }

        static std::string Stderr(const std::string& file_name)
        {
            return AddSuffix(file_name, ".stderr");
        }
    };

    class FileUtil
    {
    public:
        static bool IsFileExist(const std::string &path_name)
        {
            struct stat st;
            if (stat(path_name.c_str(), &st) == 0)
                return true;
            return false;
        }

        static std::string CreateUniqueFile()
        {
            //利用 毫秒级时间戳 + 原子递增量 组成唯一文件名
            static std::atomic_int atomic(0);
            atomic++;
            return TimeUtil::GetTimeStamp_MS() + "_" + to_string(atomic);
        }

        static bool WriteFile(const std::string& target_file, const std::string& content)
        {
            std::ofstream out(target_file);
            if (!out.is_open())
            {
                return false;
            }
            out.write(content.c_str(), content.size());
            out.close();
            return true;
        }

        static bool ReadFile(const std::string& target_file, std::string& content, bool keep = false)
        {
            content.clear();
            std::ifstream in(target_file);
            if (!in.is_open())
            {
                return false;
            }
            std::string line;
            while (getline(in, line))
            {
                content += line;
                content += (keep ? "\n" : "");
            }
            in.close();
            return true;
        }
    };

    class StringUtil
    {
    public:
        static void SplitString(const std::string& str, std::vector<std::string>& target, const std::string& sep)
        {
            boost::split(target, str, boost::is_any_of(sep), boost::algorithm::token_compress_on);
        }
    };
}