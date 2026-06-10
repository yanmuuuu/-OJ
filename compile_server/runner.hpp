#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../comm/util.hpp"
#include "../comm/log.hpp"

//运行模块
// 
//功能 : 执行已编译好的可执行文件(.exe), 捕获其标准输出和标准错误
// 
//文件约定(所有文件位于 temp/ 目录下):
//   - 标准输入：<文件名>.stdin   (当前模块无实际输入需求，重定向到空文件)
//   - 标准输出：<文件名>.stdout  (程序正常运行时输出的内容)
//   - 标准错误：<文件名>.stderr  (程序运行时的错误信息，包括崩溃时的系统错误)
//
//运行结果分类(运行模块仅关注进程是否正常结束，不判断业务逻辑正确与否):
//   1. 进程正常执行完毕(无论业务结果对错)
//   2. 进程被信号终止(如段错误、除零异常等)
//
//返回值(int):
//   < 0 : 运行模块自身出现错误（如文件打开失败, fork失败, execlp失败等)
//   = 0 : 子进程正常终止(未收到任何信号, 包括 exit(0) 和 exit(非0))
//   > 0 : 子进程被信号终止, 返回值为信号编号(如 11 表示 SIGSEGV)
//
// 注意 : 本模块不判断程序输出结果的正确性, 只关心运行过程是否发生异常

namespace ns_runner
{
    using namespace ns_log;
    using namespace ns_util;

    class Runner
    {
    public:
        Runner()
        {
        }

        ~Runner()
        {
        }

        static void SetProcLimit(int cpu_limit, int mem_limit)
        {
            struct rlimit cpu_rlimit;
            cpu_rlimit.rlim_cur = cpu_limit;
            cpu_rlimit.rlim_max = RLIM_INFINITY;
            setrlimit(RLIMIT_CPU, &cpu_rlimit);

            struct rlimit mem_rlimit;
            mem_rlimit.rlim_cur = mem_limit * 1024; //内存单位为Mb, 这里用kb, 可以表示更多情况
            mem_rlimit.rlim_max = RLIM_INFINITY;
            setrlimit(RLIMIT_AS, &mem_rlimit);
        }

        //传入文件名, cpu运行时间限制, 内存限制
        static int Run(const std::string &file_name, int cpu_limit, int mem_limit)
        {
            std::string _std_in = PathUtil::Stdin(file_name);
            std::string _std_out = PathUtil::Stdout(file_name);
            std::string _std_err = PathUtil::Stderr(file_name);

            umask(0);
            int _in_fd = open(_std_in.c_str(), O_CREAT | O_RDONLY, 0644);
            int _out_fd = open(_std_out.c_str(), O_CREAT | O_WRONLY, 0644);
            int _err_fd = open(_std_err.c_str(), O_CREAT | O_WRONLY, 0644);
            if (_in_fd < 0 || _out_fd < 0 || _err_fd < 0)
            {
                LOG(LogLevel::ERROR) << "运行模块: open失败";
                return -1; //open失败 
            }

            pid_t pid = fork();
            if (pid < 0)
            {
                close(_in_fd);
                close(_out_fd);
                close(_err_fd);
                LOG(LogLevel::ERROR) << "运行模块: fork失败";
                return -2; //创建子进程失败
            }
            else if (pid == 0)
            {
                dup2(_in_fd, 0);
                dup2(_out_fd, 1);
                dup2(_err_fd, 2);

                SetProcLimit(cpu_limit, mem_limit);

                execlp(PathUtil::Exe(file_name).c_str(), PathUtil::Exe(file_name).c_str(), nullptr);
                LOG(LogLevel::ERROR) << "运行模块: execlp异常";
                exit(1);
            }
            else
            {
                close(_in_fd);
                close(_out_fd);
                close(_err_fd);
                int status = 0;
                waitpid(pid, &status, 0);
                LOG(LogLevel::INFO) << "运行模块完成, 返回信号 : " << (status & 0x7F);
                return status & 0x7F;
            }
        }
    };
};