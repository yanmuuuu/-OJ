#pragma once

#include <iostream>
#include <mutex>
#include <cassert>
#include <fstream>
#include <vector>

#include "../../comm/log.hpp"
#include "../../comm/util.hpp"

namespace ns_control
{
    using namespace ns_log;
    using namespace ns_util;

    struct Machine
    {
        std::string _ip;
        int _port;
        uint64_t _load;
        std::mutex *_mutex;

        Machine()
            : _ip(""), _port(0), _load(0), _mutex(nullptr)
        {
        }

        void IncLoad()
        {
            if (_mutex)
                _mutex->lock();
            ++_load;
            if (_mutex)
                _mutex->unlock();
        }

        void DecLoad()
        {
            if (_mutex)
                _mutex->lock();
            --_load;
            if (_mutex)
                _mutex->unlock();
        }

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

    class LoadBlance
    {
    public:
        LoadBlance()
        {
            std::string conf_path = FileUtil::GetProjectPath("./conf/service_machine.conf");
            assert(LoadConf(conf_path));
            LOG(LogLevel::FATAL) << "文件 : " << conf_path << " 内容获取成功";
        }

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
        std::vector<Machine> _machines;
        std::vector<int> _online;
        std::vector<int> _offline;
        std::mutex _mutex;
    };
}
