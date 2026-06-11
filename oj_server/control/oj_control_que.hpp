#pragma once

#include <iostream>
#include <algorithm>

#include <jsoncpp/json/json.h>

#include "../../comm/log.hpp"
#include "../../comm/util.hpp"
#include "../../comm/httplib.h"
#include "../model/oj_model_sql_que.hpp"
#include "../model/oj_model_sql_usr.hpp"
#include "../view/oj_view.hpp"
#include "oj_control_machine.hpp"

namespace ns_control
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;
    using namespace ns_view;
    using namespace httplib;

    class ControlQue
    {
    public:
        //获取所有题目
        bool AllQuestions(std::string &html)
        {
            std::vector<Question> questions;
            if (_model_questions.GetAllQuestions(questions))
            {
                std::sort(questions.begin(), questions.end(), [](const Question &q1, const Question &q2) {
                    return stoi(q1.number) < stoi(q2.number);
                });
                _view.AllExpandHtml(questions, html);
                return true;
            }
            html = "获取全部题目失败";
            return false;
        }

        //获取单个题目
        bool OneQuestion(const std::string &number, std::string &html)
        {
            Question question;
            if (_model_questions.GetOneQuestion(number, question))
            {
                _view.OneExpandHtml(question, html);
                return true;
            }
            html = "获取指定题目失败 : " + number;
            return false;
        }

        //判题
        void Judge(const std::string &number, const std::string &in_json, std::string &out_json)
        {
            Question q;
            _model_questions.GetOneQuestion(number, q);

            Json::Value in_root;
            Json::Reader reader;
            reader.parse(in_json, in_root);

            Json::Value compile_root;
            Json::FastWriter writer;
            compile_root["input"] = in_root["input"].asString();
            compile_root["code"] = in_root["code"].asString() + '\n' + q.tail;
            compile_root["cpu_limit"] = q.cpu_limit;
            compile_root["mem_limit"] = q.mem_limit;
            std::string compile_str = writer.write(compile_root);

            while (true)
            {
                int id = 0;
                Machine *machine = nullptr;
                if (!_loadblance.SmartChoice(id, &machine))
                    break;

                Client client(machine->_ip, machine->_port);
                client.set_read_timeout(15);
                client.set_write_timeout(15);

                machine->IncLoad();
                LOG(LogLevel::INFO) << "选择服务器成功, id -> " << id << ", 具体信息 -> "
                                    << machine->_ip << " : " << machine->_port
                                    << ", 此时load -> " << machine->GetLoad() << std::endl;
                auto res = client.Post("/compile_and_run", compile_str, "application/json;charset=utf-8");
                if (res)
                {
                    if (res->status == 200)
                    {
                        out_json = res->body;
                        machine->DecLoad();
                        break;
                    }
                    machine->DecLoad();
                }
                else
                {
                    std::cerr << "HTTP 请求失败, 错误码: " << static_cast<int>(res.error()) << std::endl;
                    LOG(LogLevel::ERROR) << " 当前请求的主机id: " << id << " 详情: " << machine->_ip
                                         << ":" << machine->_port << " 可能已经离线"
                                         << ", 此时load -> " << machine->GetLoad();
                    machine->DecLoad();
                    _loadblance.OfflineMachine(id);
                }
            }
            SanitizeJudgeResponse(out_json);
        }

        //运行测试样例
        void Run(const std::string& number, const std::string& in_json, std::string& out_json)
        {
            Question q;
            _model_questions.GetOneQuestion(number, q);

            Json::Value in_root;
            Json::Reader reader;
            reader.parse(in_json, in_root);

            Json::Value compile_root;
            Json::FastWriter writer;
            compile_root["input"] = in_root["input"].asString();
            compile_root["code"] = in_root["code"].asString() + '\n' + in_root["case_code"].asString();
            compile_root["cpu_limit"] = q.cpu_limit;
            compile_root["mem_limit"] = q.mem_limit;
            std::string compile_str = writer.write(compile_root);

            while (true)
            {
                int id = 0;
                Machine *machine = nullptr;
                if (!_loadblance.SmartChoice(id, &machine))
                    break;

                Client client(machine->_ip, machine->_port);
                client.set_read_timeout(15);
                client.set_write_timeout(15);

                machine->IncLoad();
                LOG(LogLevel::INFO) << "选择服务器成功, id -> " << id << ", 具体信息 -> "
                                    << machine->_ip << " : " << machine->_port
                                    << ", 此时load -> " << machine->GetLoad() << std::endl;
                auto res = client.Post("/compile_and_run", compile_str, "application/json;charset=utf-8");
                if (res)
                {
                    if (res->status == 200)
                    {
                        out_json = res->body;
                        machine->DecLoad();
                        break;
                    }
                    machine->DecLoad();
                }
                else
                {
                    std::cerr << "HTTP 请求失败, 错误码: " << static_cast<int>(res.error()) << std::endl;
                    LOG(LogLevel::ERROR) << " 当前请求的主机id: " << id << " 详情: " << machine->_ip
                                         << ":" << machine->_port << " 可能已经离线"
                                         << ", 此时load -> " << machine->GetLoad();
                    machine->DecLoad();
                    _loadblance.OfflineMachine(id);
                }
            }
            SanitizeJudgeResponse(out_json);
        }

        //添加题目
        void SubmitQuestion(const Request& req, const std::string& in_json, std::string& out_json)
        {
            std::string session = CookieUtil::ParseSessionId(req);
            if (session.empty())
            {
                out_json = BuildAuthJson(1, "请先登录");
                return;
            }

            User user;
            if (!_model_users.GetUserBySession(session, user))
            {
                _model_users.DeleteSessionIfExpired(session);
                out_json = BuildAuthJson(1, "请先登录");
                return;
            }

            Json::Value in_root;
            Json::Reader in_reader;
            if (!in_reader.parse(in_json, in_root) ||
                !in_root.isMember("title") ||
                !in_root.isMember("star") ||
                !in_root.isMember("desc") ||
                !in_root.isMember("head") ||
                !in_root.isMember("tail") ||
                !in_root.isMember("cpu_limit") ||
                !in_root.isMember("mem_limit") || 
                !in_root.isMember("run_case"))
            {
                out_json = BuildAuthJson(2, "请求格式不合法");
                return;
            }

            Question q;
            q.title = in_root["title"].asString();
            q.star = in_root["star"].asString();
            q.desc = in_root["desc"].asString();
            q.head = in_root["head"].asString();
            q.tail = in_root["tail"].asString();
            q.cpu_limit = in_root["cpu_limit"].asInt();
            q.mem_limit = in_root["mem_limit"].asInt();
            q.run_case = in_root["run_case"].asString();

            if (q.title.empty())
            {
                out_json = BuildAuthJson(3, "题目标题不能为空");
                return;
            }
            if (q.cpu_limit <= 0 || q.mem_limit <= 0)
            {
                out_json = BuildAuthJson(3, "时间或内存限制不合法");
                return;
            }
            if (q.star != "简单" && q.star != "中等" && q.star != "困难")
            {
                out_json = BuildAuthJson(3, "难度只能选择：简单、中等、困难");
                return;
            }
            if (q.run_case.empty())
            {
                out_json = BuildAuthJson(3, "运行用例不能为空");
                return;
            }

            std::string number;
            if (!_model_questions.InsertQuestion(q, user.id, number))
            {
                out_json = BuildAuthJson(4, "系统繁忙，请稍后重试");
                return;
            }

            Json::Value data;
            data["number"] = number;
            out_json = BuildAuthJson(0, "录题成功", &data);
        }

        //录题页面
        bool SubmitQuestionPage(std::string &html)
        {
            _view.SubmitQuestionExpandHtml(html);
            return true;
        }

        //让所有服务器上线
        void RecoveryMachine()
        {
            _loadblance.OnlineMachine();
        }

    private:
        //构建添加题目 API 响应
        static std::string BuildAuthJson(int errcode, const std::string &errmsg, const Json::Value *data = nullptr)
        {
            Json::Value root;
            root["errcode"] = errcode;
            root["errmsg"] = errmsg;
            if (data != nullptr)
                root["data"] = *data;
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            builder["emitUTF8"] = true;
            return Json::writeString(builder, root);
        }

        //构建判题 API 响应
        static std::string BuildJudgeErrorJson(int status, const std::string &reason)
        {
            Json::Value root;
            root["status"] = status;
            root["reason"] = reason;
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            builder["emitUTF8"] = true;
            return Json::writeString(builder, root);
        }

        //判题请求
        static void SanitizeJudgeResponse(std::string &out_json)
        {
            if (out_json.empty())
            {
                out_json = BuildJudgeErrorJson(-2, "判题服务暂时不可用，请稍后重试");
                return;
            }

            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(out_json, root) || !root.isObject() || !root.isMember("status"))
                out_json = BuildJudgeErrorJson(-2, "判题服务暂时不可用，请稍后重试");
        }
    private:
        ModelQuestions _model_questions;
        ModelUser _model_users;
        View _view;
        LoadBlance _loadblance;
    };
}
