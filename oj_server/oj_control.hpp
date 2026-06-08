#pragma once
// 拿到数据构建网页

#include <iostream>
#include <mutex>
#include <cassert>
#include <algorithm>
#include <cstring>

#include <jsoncpp/json/json.h>

#include "../comm/log.hpp"
#include "../comm/util.hpp"
#include "oj_model_mysql.hpp"
#include "oj_view.hpp"
#include "../comm/httplib.h"

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

    const std::string &conf_path = "./conf/service_machine.conf";
    // 负载均衡
    class LoadBlance
    {
    public:
        LoadBlance()
        {
            assert(LoadConf(conf_path));
            LOG(LogLevel::FATAL) << "文件 : " << conf_path << " 内容获取成功" << std::endl;
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
                LOG(LogLevel::FATAL) << "文件 : " << conf_path << " 内容获取失败" << std::endl;
                return false;
            }
            std::string line;
            while (std::getline(in, line))
            {
                std::vector<std::string> target;
                StringUtil::SplitString(line, target, ":");
                if (target.size() != 2)
                {
                    LOG(LogLevel::WARNING) << "切割 : " << line << " 失败" << std::endl;
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
                LOG(LogLevel::FATAL) << "当前没有服务器运行!" << std::endl;
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
            LOG(INFO) << "所有的主机有上线啦!" << "\n";
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

        // 获取全部题目网页
        bool AllQuestions(std::string &html)
        {
            std::vector<Question> questions;
            if (_model.GetAllQuestions(questions))
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
            if (_model.GetOneQuestion(number, question))
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
            _model.GetOneQuestion(number, q);

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
                    LOG(ERROR) << " 当前请求的主机id: " << id << " 详情: " << machine->_ip << ":" << machine->_port << " 可能已经离线" << ", 此时load -> " << machine->GetLoad() << std::endl;
                    machine->DecLoad();
                    _loadblance.OfflineMachine(id);
                }
            }
        }

        void RecoveryMachine()
        {
            _loadblance.OnlineMachine();
        }

    private:
        Model _model;           // 提供数据
        View _view;             // 渲染网页
        LoadBlance _loadblance; // 负载均衡
    };
}
