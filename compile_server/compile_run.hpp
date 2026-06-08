#include <jsoncpp/json/json.h>

#include "compile.hpp"
#include "runner.hpp"

// 编译运行模块: 将用户提交的代码编译并执行, 返回运行结果或错误信息
//
// 输入 Json (in_json) 字段说明:
//   - code      : string, 用户的 C++ 源代码(必填)
//   - input     : string, 程序的标准输入内容(可选，当前版本未使用，保留扩展)
//   - cpu_limit : int,    CPU 时间限制(s), 用于限制运行时间(必填)
//   - mem_limit : int,    内存限制(MB)，用于限制内存使用(必填)
//
// 输出 Json (out_json) 字段说明:
//   - status : int,    状态码(详见 CodeToDesc 映射)
//   - reason : string, 状态码对应的描述
//   - stdout : string, 当 status == 0 时返回，程序的标准输出内容
//   - stderr : string, 当 status == 0 时返回，程序的标准错误内容

namespace ns_compile_run
{
    using namespace ns_compiler;
    using namespace ns_runner;
    using namespace ns_log;
    using namespace ns_util;

    class Compile_run
    {
    public:
        Compile_run()
        {
        }

        ~Compile_run()
        {
        }

        static void RemoveTempFile(const std::string &file_name)
        {
            if (FileUtil::IsFileExist(PathUtil::Src(file_name)))
                unlink(PathUtil::Src(file_name).c_str());
            if (FileUtil::IsFileExist(PathUtil::Compile_err(file_name)))
                unlink(PathUtil::Compile_err(file_name).c_str());
            if (FileUtil::IsFileExist(PathUtil::Exe(file_name)))
                unlink(PathUtil::Exe(file_name).c_str());
            if (FileUtil::IsFileExist(PathUtil::Stderr(file_name)))
                unlink(PathUtil::Stderr(file_name).c_str());
            if (FileUtil::IsFileExist(PathUtil::Stdin(file_name)))
                unlink(PathUtil::Stdin(file_name).c_str());
            if (FileUtil::IsFileExist(PathUtil::Stdout(file_name)))
                unlink(PathUtil::Stdout(file_name).c_str());
        }

        static std::string CodeToDesc(int status_code, const std::string &file_name)
        {
            std::string desc;
            switch (status_code)
            {
            case 0:
                desc = "运行成功";
                break;
            case -1:
                desc = "代码为空";
                break;
            case -2:
                desc = "未知错误";
                break;
            case -3:
                FileUtil::ReadFile(PathUtil::Compile_err(file_name), desc, true);
                break;
            case SIGFPE: // 8
                desc = "浮点数溢出";
                break;
            case SIGXCPU: // 24
                desc = "超出时间限制";
                break;
            case SIGABRT: // 6
                desc = "超出空间限制";
                break;
            default:
                desc = "unknow err : " + std::to_string(status_code);
                break;
            }
            return desc;
        }

        static void Start(const std::string &in_json, std::string &out_json)
        {
            Json::Value in_root;
            Json::Reader in_reader;

            in_reader.parse(in_json, in_root);

            // goto语句不允许创建新变量, 所以需要先定义
            int status_code = 0;
            std::string file_name;
            int runner_code;

            std::string code = in_root["code"].asString();
            std::string input = in_root["input"].asString();
            int cpu_limit = in_root["cpu_limit"].asInt();
            int mem_limit = in_root["mem_limit"].asInt();

            if (code.empty())
            {
                status_code = -1; // 代码为空
                goto END;
            }

            // 这里只会生成不重复, 无后缀的文件
            file_name = FileUtil::CreateUniqueFile();

            // 将用户提交的代码写入 [file_name].cc 文件里面
            if (!FileUtil::WriteFile(PathUtil::Src(file_name), code))
            {
                status_code = -2; // 写入错误(属于服务器错误一律属于未知错误)
                goto END;
            }

            // 编译
            if (!Compiler::Compile(file_name))
            {
                status_code = -3; // 编译错误
                goto END;
            }

            // 运行
            runner_code = Runner::Run(file_name, cpu_limit, mem_limit);
            if (runner_code < 0)
            {
                status_code = -2;
                goto END;
            }
            else if (runner_code > 0)
            {
                status_code = runner_code; // 使用运行错误返回的信号量
                goto END;
            }
            else
            {
                status_code = 0; // 成功运行
                goto END;
            }

        END:
            Json::Value out_root;
            //Json::StyledWriter out_writer; 当前Jsoncpp版本不支持直接使用中文写入了
            Json::StreamWriterBuilder writer_builder;

            out_root["status"] = status_code;
            out_root["reason"] = CodeToDesc(status_code, file_name);
            if (status_code == 0)
            {
                std::string stdout_str;
                FileUtil::ReadFile(PathUtil::Stdout(file_name), stdout_str, true);
                out_root["stdout"] = stdout_str;

                std::string stderr_str;
                FileUtil::ReadFile(PathUtil::Stderr(file_name), stderr_str, true);
                out_root["stderr"] = stderr_str;
            }

            //out_json = out_writer.write(out_root);

            writer_builder["indentation"] = "  "; //不要空格->类似FastWriter, 要空格->类似StyledWriter
            writer_builder["emitUTF8"] = true;  //保留原始 UTF-8 中文，不转义为 \uXXXX
            out_json = Json::writeString(writer_builder, out_root);

            RemoveTempFile(file_name);
        }
    };
};
