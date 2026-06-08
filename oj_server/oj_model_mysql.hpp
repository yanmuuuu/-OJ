#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>
#include <assert.h>

#include "../comm/log.hpp"
#include "include/mysql.h"

//根据questions.list将所有题目文件添加到内存中
//该模块用来和数据交互,将数据交给外部

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_util;

    const std::string oj_questions = "oj_questions";
    const std::string host = "127.0.0.1";
    const std::string user = "oj_backend";
    const std::string passwd = "123456";
    const std::string db = "oj";
    const int port = 3306;

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

    class Model
    {
    public:
        Model()
        {
        }

        ~Model()
        {}

        bool QueryMysql(const std::string& sql, std::vector<Question>& out)
        {
            //初始化数据库
            MYSQL* mysql = mysql_init(nullptr);
            if (mysql == nullptr)
            {
                LOG(LogLevel::FATAL) << "mysql_init failed" << std::endl;
                return false;
            }

            //设置编码格式
            mysql_set_character_set(mysql, "utf8");

            //连接数据库
            if (mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
            {
                LOG(LogLevel::FATAL) << "mysql_real_connet failed" << std::endl;
                mysql_close(mysql);
                return false;
            }
            LOG(LogLevel::INFO) << "connect mysql success" << std::endl;
            

            //执行MySql语句
            if (mysql_query(mysql, sql.c_str()) != 0)
            {
                LOG(LogLevel::WARNING) << "mysql_query failed, sql : " << sql << std::endl;
                mysql_close(mysql);
                return false;
            }

            //分析结果
            MYSQL_RES* result = mysql_store_result(mysql);
            if (result == nullptr)      
            {
                LOG(LogLevel::WARNING) << "mysql_store_result failed, sql : " << sql << std::endl;
                mysql_close(mysql);
                return false;
            }
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)) != nullptr)
            {
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
            }      
            mysql_free_result(result);

            //关闭连接
            mysql_close(mysql);

            return true;
        }

    
        bool GetAllQuestions(std::vector<Question>& out)
        {
            std::string sql;
            sql += "select * from " + oj_questions;
            return QueryMysql(sql, out);
        }

        bool GetOneQuestion(const std::string& number, Question& q)
        {
            std::string sql;
            sql += "select * from " + oj_questions + " where number=" + number;
            std::vector<Question> out;
            if (QueryMysql(sql, out))
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

