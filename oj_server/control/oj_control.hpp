#pragma once
// 拿到数据构建网页

#include <iostream>
#include <mutex>
#include <cassert>
#include <algorithm>
#include <cstring>

#include <jsoncpp/json/json.h>

#include "../../comm/log.hpp"
#include "../../comm/util.hpp"
#include "../model/oj_model_sql_que.hpp"
#include "../model/oj_model_sql_usr.hpp"
#include "../view/oj_view.hpp"
#include "../../comm/httplib.h"

namespace ns_control
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;
    using namespace ns_view;
    using namespace httplib;

    // 提供编译服务的主机
    struct Machine
    {
        std::string _ip;    // 主机ip
        int _port;          // 主机port
        uint64_t _load;     // 主机承载的服务数量, 是共享资源, 需要增加减少, 所以加个锁
        std::mutex *_mutex; // 由于c++Muext锁无法拷贝, 所以我们使用指针来拷贝

        Machine()
            : _ip(""), _port(0), _load(0), _mutex(nullptr)
        {
        }

        ~Machine()
        {
        }

        // 增加负载
        void IncLoad()
        {
            if (_mutex)
                _mutex->lock();
            ++_load;
            if (_mutex)
                _mutex->unlock();
        }

        // 减少负载
        void DecLoad()
        {
            if (_mutex)
                _mutex->lock();
            --_load;
            if (_mutex)
                _mutex->unlock();
        }

        // 获取负载, 主要是接口统一, 毕竟读取共享资源不需要加锁
        uint64_t GetLoad()
        {
            uint64_t load;
            if (_mutex)
                _mutex->lock();
            load = _load;
            if (_mutex)
                _mutex->unlock();
            return load;
        }

        void ResetLoad()
        {
            if (_mutex)
                _mutex->lock();
            _load = 0;
            if (_mutex)
                _mutex->unlock();
        }
    };

    // 负载均衡
    class LoadBlance
    {
    public:
        LoadBlance()
        {
            std::string conf_path = FileUtil::GetProjectPath("./conf/service_machine.conf");
            assert(LoadConf(conf_path));
            LOG(LogLevel::FATAL) << "文件 : " << conf_path << " 内容获取成功";
        }

        ~LoadBlance()
        {
        }

        // 加载
        bool LoadConf(const std::string &conf_path)
        {
            std::ifstream in(conf_path);
            if (!in.is_open())
            {
                LOG(LogLevel::FATAL) << "文件 : " << conf_path << " 内容获取失败";
                return false;
            }
            std::string line;
            while (std::getline(in, line))
            {
                std::vector<std::string> target;
                StringUtil::SplitString(line, target, ":");
                if (target.size() != 2)
                {
                    LOG(LogLevel::WARNING) << "切割 : " << line << " 失败";
                    continue;
                }
                Machine m;
                m._ip = target[0];
                m._port = std::stoi(target[1]);
                m._load = 0;
                m._mutex = new std::mutex();
                _online.push_back(_machines.size());
                _machines.push_back(m);
            }
            in.close();
            return true;
        }

        // 轮询式负载均衡
        bool SmartChoice(int &id, Machine **machine)
        {
            _mutex.lock();
            int online_num = _online.size();
            if (online_num == 0)
            {
                LOG(LogLevel::FATAL) << "当前没有服务器运行!";
                _mutex.unlock();
                return false;
            }
            id = _online[0];
            uint64_t min_load = _machines[_online[0]].GetLoad();
            *machine = &_machines[_online[0]];
            for (int i = 1; i < online_num; i++)
            {
                uint64_t load = _machines[_online[i]].GetLoad();
                if (min_load > load)
                {
                    min_load = load;
                    id = _online[i];
                    *machine = &_machines[_online[i]];
                }
            }
            _mutex.unlock();
            return true;
        }

        void OfflineMachine(int id)
        {
            _mutex.lock();
            for (auto iter = _online.begin(); iter != _online.end(); iter++)
            {
                if (*iter == id)
                {
                    _machines[id].ResetLoad();
                    _online.erase(iter);
                    _offline.push_back(id);
                    break;
                }
            }
            _mutex.unlock();
        }

        void OnlineMachine()
        {
            _mutex.lock();
            _online.insert(_online.end(), _offline.begin(), _offline.end());
            _offline.erase(_offline.begin(), _offline.end());
            _mutex.unlock();
            LOG(LogLevel::INFO) << "所有的主机有上线啦!";
        }

    private:
        // 所有主机
        std::vector<Machine> _machines;
        // 在线主机id, 使用id映射所有主机下标
        std::vector<int> _online;
        // 离线主机id, 使用id映射所有主机下标
        std::vector<int> _offline;
        // 为了保护负载均衡模块(Machine就是共享资源), 所以加个锁
        std::mutex _mutex;
    };

    // 服务主要控制器
    class Control
    {
    public:
        Control()
        {
        }

        ~Control()
        {
        }

        // 注册
        void Register(const std::string &in_json, std::string &out_json)
        {
            Json::Value in_root;
            Json::Reader in_reader;
            if (!in_reader.parse(in_json, in_root) ||
                !in_root.isMember("username") ||
                !in_root.isMember("password") ||
                !in_root.isMember("confirm_password"))
            {
                out_json = BuildAuthJson(3, "请求格式不合法");
                return;
            }

            std::string username = in_root["username"].asString();
            std::string password = in_root["password"].asString();
            std::string confirm_password = in_root["confirm_password"].asString();

            std::string errmsg;
            int errcode = 0;

            if (!ValidateUsernameStrength(username, errmsg, errcode))
            {
                out_json = BuildAuthJson(errcode, errmsg);
                return;
            }

            if (!ValidatePasswordStrength(password, errmsg, errcode))
            {
                out_json = BuildAuthJson(errcode, errmsg);
                return;
            }

            if (password != confirm_password)
            {
                out_json = BuildAuthJson(2, "两次输入的密码不一致");
                return;
            }

            std::string crypto_password = CryptoUtil::HashPassword(password);
            if (crypto_password.empty() || !_model_users.InsertUser(username, crypto_password, username))
            {
                out_json = BuildAuthJson(4, "系统繁忙，请稍后重试");
                return;
            }
            LOG(LogLevel::INFO) << "用户名 : " << username << " 注册成功";
            out_json = BuildAuthJson(0, "注册成功");
        }

        //登录
        void Login(const std::string &in_json, std::string &out_json, std::string &set_cookie)
        {
            set_cookie.clear();
            Json::Value in_root;
            Json::Reader in_reader;
            if (!in_reader.parse(in_json, in_root) ||
                !in_root.isMember("username") ||
                !in_root.isMember("password"))
            {
                out_json = BuildAuthJson(3, "请求格式不合法");
                return;
            }
            std::string username = in_root["username"].asString();
            std::string password = in_root["password"].asString();
            std::string errmsg;
            int errcode = 0;
            if (!ValidateUsernameBasic(username, errmsg, errcode))
            {
                out_json = BuildAuthJson(errcode, errmsg);
                return;
            }

            if (!ValidatePasswordStrength(password, errmsg, errcode))
            {
                out_json = BuildAuthJson(errcode, errmsg);
                return;
            }

            UserCredential uc;
            if (!_model_users.GetUserByName(username, uc) ||
                !CryptoUtil::VerifyPassword(password, uc.password_hash))
            {
                out_json = BuildAuthJson(1, "用户名或密码错误");
                return;
            }

            std::string session_id = CryptoUtil::RandomHex(32);
            if (session_id.empty() ||
                !_model_users.CreateSession(session_id, uc.user.id, SessionExpireAt()))
            {
                out_json = BuildAuthJson(4, "系统繁忙，请稍后重试");
                return;
            }
            set_cookie = "session_id=" + session_id
                        + "; Path=/; HttpOnly; SameSite=Lax; Max-Age=604800";
            Json::Value data;
            data["username"] = uc.user.username;
            data["nickname"] = uc.user.nickname;
            LOG(LogLevel::INFO) << "用户名 : " << username << "登录成功";
            out_json = BuildAuthJson(0, "登录成功", &data);
        }                 

        // 登出
        void Logout(const Request &req, std::string &out_json, std::string &set_cookie)
        {
            std::string session_id = ParseSessionId(req);
            if (!session_id.empty())
                _model_users.DeleteSession(session_id);
            set_cookie = "session_id=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0";
            out_json = BuildAuthJson(0, "已退出");
        }

        // 登录页
        bool LoginPage(std::string &html)
        {
            _view.LoginExpandHtml(html);
            return true;
        }

        // 注册页
        bool RegisterPage(std::string &html)
        {
            _view.RegisterExpandHtml(html);
            return true;
        }

        // 获取当前登录用户信息
        void Me(const Request &req, std::string &out_json)
        {
            _model_users.DeleteExpiredSessions();

            std::string session_id = ParseSessionId(req);
            if (session_id.empty())
            {
                out_json = BuildAuthJson(0, "已退出"); //无data字段表示没登陆
                return;
            }
            
            User user;
            if (!_model_users.GetUserBySession(session_id, user))
            {
                _model_users.DeleteSessionIfExpired(session_id);
                out_json = BuildAuthJson(0, "已退出"); //无data字段表示没登陆
                return;
            }

            Json::Value data;
            data["username"] = user.username;
            data["nickname"] = user.nickname;
            out_json = BuildAuthJson(0, "ok", &data);
        }

        // 获取全部题目网页
        bool AllQuestions(std::string &html)
        {
            std::vector<Question> questions;
            if (_model_questions.GetAllQuestions(questions))
            {
                std::sort(questions.begin(), questions.end(), [](const Question &q1, const Question &q2)
                          { return stoi(q1.number) < stoi(q2.number); });
                _view.AllExpandHtml(questions, html);
                return true;
            }
            else
            {
                html = "获取全部题目失败";
                return false;
            }
        }

        // 获取指定题目网页
        bool OneQuestion(const std::string &number, std::string &html)
        {
            Question question;
            if (_model_questions.GetOneQuestion(number, question))
            {
                _view.OneExpandHtml(question, html);
                return true;
            }
            else
            {
                html = "获取指定题目失败 : " + number;
                return false;
            }
        }

        // 判题
        void Judge(const std::string &number, const std::string &in_json, std::string &out_json)
        {
            // 1.获取指定题目详细信息
            Question q;
            _model_questions.GetOneQuestion(number, q);

            // 2.读取客户发送的in_json串
            Json::Value in_root;
            Json::Reader reader;
            reader.parse(in_json, in_root);

            // 3.将所有内容写入要发送到远端服务器执行compile的compile_str字符串
            Json::Value compile_root;
            Json::FastWriter writer;
            compile_root["input"] = in_root["input"].asString();
            compile_root["code"] = in_root["code"].asString() + '\n' + q.tail;
            compile_root["cpu_limit"] = q.cpu_limit;
            compile_root["mem_limit"] = q.mem_limit;
            std::string compile_str = writer.write(compile_root);

            // 4.负载均衡
            while (true)
            {
                // 5.选择服务器
                int id = 0;
                Machine *machine = nullptr;
                if (!_loadblance.SmartChoice(id, &machine))
                {
                    // 说明此时没有任何一台服务器在线
                    break;
                }
                Client client(machine->_ip, machine->_port);

                client.set_read_timeout(15);
                client.set_write_timeout(15);

                machine->IncLoad();
                LOG(LogLevel::INFO) << "选择服务器成功, id -> " << id << ", 具体信息 -> " << machine->_ip << " : " << machine->_port << ", 此时load -> " << machine->GetLoad() << std::endl;
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
                    std::cerr << "HTTP 请求失败, 错误码: " << static_cast<int>(res.error())
                              << std::endl;
                    LOG(LogLevel::ERROR) << " 当前请求的主机id: " << id << " 详情: " << machine->_ip << ":" << machine->_port << " 可能已经离线" << ", 此时load -> " << machine->GetLoad();
                    machine->DecLoad();
                    _loadblance.OfflineMachine(id);
                }
            }
            SanitizeJudgeResponse(out_json);
        }

        void RecoveryMachine()
        {
            _loadblance.OnlineMachine();
        }

    private:
        // 判题用的辅助函数
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

        static void SanitizeJudgeResponse(std::string &out_json)
        {
            // 仅处理 oj_server 无法连通 compile_server 的情况，其余原样透传 compile_run 的 JSON
            if (out_json.empty())
            {
                out_json = BuildJudgeErrorJson(-2, "判题服务暂时不可用，请稍后重试");
                return;
            }

            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(out_json, root) || !root.isObject() || !root.isMember("status"))
            {
                out_json = BuildJudgeErrorJson(-2, "判题服务暂时不可用，请稍后重试");
            }
        }

    private:
        // 登陆, 注册, 登出, 选择账户 用的辅助函数
        // 从 HTTP 请求的 Cookie 头中提取 session_id
        std::string ParseSessionId(const Request &req)
        {
            std::string cookie = req.get_header_value("Cookie");
            const std::string key = "session_id=";
            size_t pos = cookie.find(key);
            if (pos == std::string::npos)
                return "";
            pos += key.size();
            size_t end = cookie.find(';', pos);
            if (end == std::string::npos)
                return cookie.substr(pos);
            return cookie.substr(pos, end - pos);
        }

        // 生成统一的认证响应 JSON 格式，包含错误码、错误信息和可选的捎带数据
        std::string BuildAuthJson(int errcode, const std::string &errmsg, const Json::Value *data = nullptr)
        {
            Json::Value root;
            root["errcode"] = errcode;
            root["errmsg"] = errmsg;
            if (data != nullptr)
            {
                root["data"] = *data;
            }
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            builder["emitUTF8"] = true;         
            return Json::writeString(builder, root);
        }

        // 返回会话过期的绝对时间，用于设置 Cookie 的 Max-Age 或存储时的过期字段
        std::string SessionExpireAt()
        {
            time_t expire = time(nullptr) + 7 * 24 * 3600; //7天
            struct tm t;
            localtime_r(&expire, &t);
            char buf[20];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
            return std::string(buf);
        }

        // 校验用户名合法性, 当前只检验长度
        bool ValidateUsernameBasic(const std::string &username, std::string &errmsg, int &errcode)
        {
            // 长度
            if (username.size() < 3 || username.size() > 16)
            {
                errmsg = "用户名长度不合规";
                errcode = 3;
                return false;
            }
            return true;
        }

        // 严格校验用户名合法性
        bool ValidateUsernameStrength(const std::string &username, std::string &errmsg, int &errcode)
        {
            // 1.检验长度
            if (!ValidateUsernameBasic(username, errmsg, errcode))
            {
                return false;
            }
            // 2.是否已经存在
            if (_model_users.UsernameExists(username))
            {
                errmsg = "该用户名已存在";
                errcode = 1;
                return false;
            }
            return true;
        }

        // 校验密码强度（最小长度、是否包含数字/字母/特殊字符等）
        // 只校验密码最基本的长度
        bool ValidatePasswordBasic(const std::string &password, std::string &errmsg, int &errcode)
        {
            if (password.size() < 8 || password.size() > 65)
            {
                errmsg = "密码长度不合规";
                errcode = 3;
                return false;
            }
            return true;
        }
        // 严格校验密码
        bool ValidatePasswordStrength(const std::string &password, std::string &errmsg, int &errcode)
        {
            // 1.长度
            if (!ValidatePasswordBasic(password, errmsg, errcode))
            {
                return false;
            }

            // 2.包含至少两种特殊字符
            int has_upper = 0;
            int has_lower = 0;
            int has_digit = 0;
            int has_special = 0;
            const std::string special_chars = "!@#$%^&*()_+-=[]{};':\",./<>?\\|`~";
            for (char c : password)
            {
                if (isupper(c))
                    has_upper = 1;
                else if (islower(c))
                    has_lower = 1;
                else if (isdigit(c))
                    has_digit = 1;
                else if (special_chars.find(c) != std::string::npos)
                    has_special = 1;
                else
                {
                    errcode = 3;
                    errmsg = "密码含有未定义符号(" + c + ')';
                    return false;
                }
            }
            if ((has_upper + has_lower + has_digit + has_special) < 2)
            {
                errcode = 3;
                errmsg = "密码必须包含至少两种类型(大写字母、小写字母、数字或特殊字符)";
                return false;
            }
            return true;
        }

    private:
        ModelQuestions _model_questions; // 提供题目数据
        ModelUsers _model_users;         // 提供用户数据
        View _view;                      // 渲染网页
        LoadBlance _loadblance;          // 负载均衡
    };
}
