#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>
#include <assert.h>

#include "../../comm/log.hpp"
#include "../../comm/util.hpp"
#include "../../third_party/include/mysql.h"

//根据questions.list将所有题目文件添加到内存中
//该模块用来和数据交互,将数据交给外部

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_util;

    struct Question
    {
        std::string number;    //题目编号
        std::string title;     //题目标题
        std::string star;      //题目难度(简单 中等 困难)
        std::string desc;      //题目描述
        std::string head;      //用户代码
        std::string tail;      //系统代码
        int cpu_limit;         //时间限制
        int mem_limit;         //空间限制
    };

    class ModelQuestions
    {
    public:
        ModelQuestions()
        {
        }

        ~ModelQuestions()
        {}

        bool QueryMysqlSelect(const std::string& sql, std::vector<Question>& out)
        {               
            return MysqlUtil::Query(sql, [&out](MYSQL_ROW row) -> bool{
                Question q;
                q.number = row[0];
                q.title = row[1];
                q.star = row[2];
                q.desc = row[3];
                q.head = row[4];
                q.tail = row[5];
                q.cpu_limit = atoi(row[6]);
                q.mem_limit = atoi(row[7]);
                out.push_back(q);
                return true;
            });
        }

        bool QueryMysqlOther(const std::string& sql)
        {
            return MysqlUtil::Execute(sql);
        }

    
        bool GetAllQuestions(std::vector<Question>& out)
        {
            std::string sql;
            sql += "select * from " + oj_questions;
            return QueryMysqlSelect(sql, out);
        }

        bool GetOneQuestion(const std::string& number, Question& q)
        {
            std::string sql;
            sql += "select * from " + oj_questions + " where number=" + number;
            std::vector<Question> out;
            if (QueryMysqlSelect(sql, out))
            {
                if (out.size() == 1)
                {
                    q = out[0];
                    return true;
                }              
            }
            return false;
        }
    };
}

