#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "../comm/log.hpp"
#include "../comm/util.hpp"

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_util;

    struct User
    {
        int id = 0;
        std::string username;
        std::string nickname;
    };

    // 登录校验用：Control 层比对 hash，Model 只负责读库
    struct UserCredential
    {
        User user;
        std::string password_hash; // 库里的 password 字段
    };

    class ModelUsers
    {
    public:
        //判断用户名是否存在
        bool UsernameExists(const std::string &username)
        {
            std::string sql = "SELECT COUNT(*) FROM " + oj_users + " WHERE username = '" + MysqlUtil::Escape(username) + "'";
            int count = 0;
            bool success = MysqlUtil::Query(sql, [&count](MYSQL_ROW row) -> bool {
                count = atoi(row[0]);
                return true; 
            });
            if (!success)
            {
                LOG(LogLevel::DEBUG) << "该用户名 : " << username << "查找失败: " << sql;
                return false; // 查询失败，暂时当作用户名不存在处理
            }
            return count > 0;
        }

        //注册用户
        bool InsertUser(const std::string &username, const std::string &password_hash, const std::string &nickname)
        {
            std::string sql = "INSERT INTO " 
                            + oj_users 
                            + " (username, password, nickname) VALUES ('" 
                            + MysqlUtil::Escape(username) + "', '" 
                            + MysqlUtil::Escape(password_hash) 
                            + "', '" + MysqlUtil::Escape(nickname) 
                            + "')";
            return MysqlUtil::Execute(sql);
        }

        //通过登录名找到用户信息
        bool GetUserByName(const std::string &username, UserCredential &out)
        {
            bool found = false;
            std::string sql = "SELECT * FROM " 
                            + oj_users 
                            + " WHERE username = '" 
                            + MysqlUtil::Escape(username) 
                            + "'";
            bool success = MysqlUtil::Query(sql, [&out, &found](MYSQL_ROW row) -> bool {
                found = true;
                User user;
                user.id = std::stoi(row[0]);
                user.username = row[1];
                user.nickname = row[3];
                out.user = user;
                out.password_hash = row[2];
                return false; 
            });
            return success && found;
        }

        //向数据库表oj_sessions插入一条session
        bool CreateSession(const std::string &session_id, int user_id, const std::string &expire_at) // expire_at -> 过期时间
        {
            std::string sql = "INSERT INTO " 
                            + oj_sessions 
                            + "(session_id, user_id, expire_at) VALUES ('" 
                            + MysqlUtil::Escape(session_id) 
                            + "', '" 
                            + std::to_string(user_id) 
                            + "', '" + MysqlUtil::Escape(expire_at) 
                            + "')";
            return MysqlUtil::Execute(sql);
        }

        //通过seesion_id找到用户
        bool GetUserBySession(const std::string &session_id, User &out)
        {
            std::string sql = "SELECT u.id, u.username, u.nickname FROM " 
                            + oj_sessions 
                            + " s JOIN " 
                            + oj_users 
                            + " u ON s.user_id = u.id WHERE s.session_id = '" 
                            + MysqlUtil::Escape(session_id) 
                            + "' AND s.expire_at > NOW() LIMIT 1";
            bool found = false;
            bool success = MysqlUtil::Query(sql, [&out, &found](MYSQL_ROW row) -> bool {
                found = true;
                out.id = std::stoi(row[0]);
                out.username = row[1];
                out.nickname = row[2];
                return false; 
            });
            return success && found;
        }

        //从数据库表oj_sessions中删除一条session
        bool DeleteSession(const std::string &session_id)
        {
            std::string sql = "DELETE FROM " 
                            + oj_sessions
                            + " WHERE session_id = '" 
                            + MysqlUtil::Escape(session_id)
                            + "'";
            return MysqlUtil::Execute(sql);
        }

        // 清理所有已过期的 session 行
        bool DeleteExpiredSessions()
        {
            std::string sql = "DELETE FROM " + oj_sessions + " WHERE expire_at <= NOW()";
            return MysqlUtil::Execute(sql);
        }

        // 删除指定 session_id 且已过期的行
        bool DeleteSessionIfExpired(const std::string &session_id)
        {
            std::string sql = "DELETE FROM " + oj_sessions
                            + " WHERE session_id = '" + MysqlUtil::Escape(session_id)
                            + "' AND expire_at <= NOW()";
            return MysqlUtil::Execute(sql);
        }
    };
}