#pragma once

#include <iostream>
#include <string>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "../comm/util.hpp"
#include "../comm/log.hpp"

// 编译模块：
// 输入：一个无后缀的 C++ 源文件（例如 "1234"），其内容为 C++ 代码
// 过程：将该文件视为 C++ 源文件进行编译（添加后缀为 1234.cc），生成可执行文件 1234.exe
// 输出：所有输出文件均放入 "temp" 目录中
//       - 1234.exe   (编译成功时生成)
//       - 1234.stderr(编译错误信息，无论成功或失败都会写入)
// 返回值：编译成功返回 true，失败返回 false

namespace ns_compiler
{

    using namespace ns_util;
    using namespace ns_log;

    class Compiler
    {
    public:
        Compiler()
        {
        }

        ~Compiler()
        {
        }

        // 编译函数
        // file_name不包含后缀
        static bool Compile(const std::string &file_name)
        {
            pid_t pid = fork();
            if (pid < 0)
            {
                LOG(LogLevel::ERROR) << "子进程创建失败, 程序退出";
                return false;
            }
            else if (pid == 0)
            {
                umask(0);
                //打开stderr文件
                int _stderr = open(PathUtil::Compile_err(file_name).c_str(), O_CREAT | O_WRONLY, 0644);
                if (_stderr < 0)
                {
                    LOG(LogLevel::ERROR) << "stderr文件创建失败";
                    exit(1);
                }

                //将标准错误重定向到stderr文件
                dup2(_stderr, 2);

                //使用编译器完成编译
                execlp("g++", "g++", PathUtil::Src(file_name).c_str(), "-o",
                        PathUtil::Exe(file_name).c_str(), "-std=c++17", "-D", "COMPILER_ONLINE", nullptr);
                
                //exit(2) 是为了在 execlp 失败时主动终止子进程
                LOG(LogLevel::ERROR) << "g++编译器异常, 可能是参数传递有误";
                exit(2);
            }
            else
            {
                waitpid(pid, nullptr, 0);
                if (FileUtil::IsFileExist(PathUtil::Exe(file_name).c_str()))
                {
                    LOG(LogLevel::INFO) << PathUtil::Src(file_name).c_str() << "编译成功";
                    return true;
                }
            }

            LOG(LogLevel::ERROR) << PathUtil::Src(file_name).c_str() <<  "编译失败";
            return false;
        }
    };
}