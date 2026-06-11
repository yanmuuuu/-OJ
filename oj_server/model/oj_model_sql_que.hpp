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

    //题目信息
    struct Question
    {
        std::string number;             //题目编号
        std::string title;              //题目标题
        std::string star;               //题目难度(简单 中等 困难)
        std::string desc;               //题目描述
        std::string head;               //用户代码
        std::string tail;               //系统代码
        int cpu_limit;                  //时间限制
        int mem_limit;                  //空间限制
        int author_id = 0;              //该题目由谁提供, 0 表示官方
        std::string author_name;        //出题目的用户名 : nickname(如果没有nickname用username)
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
                q.author_id = atoi(row[8]);
                q.author_name = row[9];
                out.push_back(q);
                return true;
            });
        }

        bool QueryMysqlOther(const std::string& sql)
        {
            return MysqlUtil::Execute(sql);
        }

        //获取全部题目信息
        bool GetAllQuestions(std::vector<Question>& out)
        {
            std::string sql;
            sql += "SELECT q.*, COALESCE(NULLIF(u.nickname,''), u.username, '官方') AS author_name FROM "
                + oj_questions
                + " q LEFT JOIN oj_users u ON q.author_id = u.id";
            return QueryMysqlSelect(sql, out);
        }

        //获取一个题目信息
        bool GetOneQuestion(const std::string& number, Question& q)
        {
            std::string sql;
            sql += "SELECT q.*, COALESCE(NULLIF(u.nickname,''), u.username, '官方') AS author_name FROM "
                + oj_questions
                + " q LEFT JOIN oj_users u ON q.author_id = u.id WHERE q.number = '"
                + MysqlUtil::Escape(number) + "'";
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

        //插入一个题目，成功时通过 out_number 返回新题号
        bool InsertQuestion(const Question& q, int author_id, std::string& out_number)
        {
            std::string sql;
            sql += "INSERT INTO "
                + oj_questions
                + " (title, star, `desc`, header, tail, cpu_limit, mem_limit, author_id) VALUES ("
                + "'" + MysqlUtil::Escape(q.title) + "'"
                + ", '" + MysqlUtil::Escape(q.star) + "'"
                + ", '" + MysqlUtil::Escape(q.desc) + "'"
                + ", '" + MysqlUtil::Escape(q.head) + "'"
                + ", '" + MysqlUtil::Escape(q.tail) + "'"
                + ", " + std::to_string(q.cpu_limit)
                + ", " + std::to_string(q.mem_limit)
                + ", " + std::to_string(author_id)
                + ")";

            unsigned long long insert_id = 0;
            if (!MysqlUtil::ExecuteInsert(sql, insert_id))
                return false;

            out_number = std::to_string(insert_id);
            return true;
        }
    };
}

