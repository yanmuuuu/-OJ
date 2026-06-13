#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>
#include <assert.h>

#include "../../comm/log.hpp"
#include "../../comm/util.hpp"

//根据questions.list将所有题目文件添加到内存中
//该模块用来和数据交互,将数据交给外部

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_util;

    static const std::string question_list = "../questions/questions.list";
    static const std::string question_path = "../questions/";

    struct Question
    {
        std::string number;    //题目编号
        std::string title;     //题目标题
        std::string desc;      //题目描述
        std::string star;      //题目难度(简单 中等 困难)
        int cpu_limit;         //时间限制
        int mem_limit;         //空间限制
        std::string head;      //用户代码
        std::string tail;      //系统代码
    };

    class ModelQuestions
    {
    public:
        ModelQuestions()
        {
            assert(LoadQuestionList(question_list));
            LOG(LogLevel::INFO) << "题库加载成功";
        }

        ~ModelQuestions()
        {}

        bool LoadQuestionList(const std::string& question_list)
        {
            //加载题目信息到_questions中
            std::ifstream in(question_list);
            if (!in.is_open())
            {
                LOG(LogLevel::FATAL) << "题库加载失败, 请检查是否有此文件";
                return false;
            }
            std::string line;
            while (getline(in, line))
            {
                std::vector<std::string> target;
                StringUtil::SplitString(line, target, " ");
                if (target.size() != 5)
                {
                    LOG(LogLevel::WARNING) << "该行有问题 : " << line;
                    continue;
                }
                Question q;
                q.number = target[0];
                q.title = target[1];
                q.star = target[2];
                q.cpu_limit = stoi(target[3]);
                q.mem_limit = stoi(target[4]);
                std::string path = question_path + q.number + "/";
                FileUtil::ReadFile(path + "desc.txt", q.desc, true);
                FileUtil::ReadFile(path + "head.cpp", q.head, true);
                FileUtil::ReadFile(path + "tail.cpp", q.tail, true);
                _questions[q.number] = q;
            }
            return true;
        }

        bool GetAllQuestions(std::vector<Question>& out)
        {
            if (_questions.empty())
            {
                LOG(LogLevel::ERROR) << "用户获取题库失败";
                return false;
            }
            for (auto& q : _questions)
            {
                out.push_back(q.second);
            }
            return true;
        }

        bool GetOneQuestion(const std::string& number, Question& q)
        {
            auto iter = _questions.find(number);
            if (iter == _questions.end())
            {
                LOG(LogLevel::ERROR) << "用户获取题目失败 : " << number;
                return false;
            }
            q = iter->second;
            return true;
        }

    private:
        std::unordered_map<std::string, Question> _questions;
    };
}

