#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <atomic>
#include <iomanip>
#include <functional>
#include <time.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "boost/algorithm/string.hpp"
#include "../third_party/include/mysql.h"
#include "httplib.h"
#include "log.hpp"

#include <argon2.h>

namespace ns_util
{
    using namespace ns_log;
    using namespace httplib;

    static const std::string temp = "./temp/";
    static const std::string oj_questions = "oj_questions";
    static const std::string oj_users = "oj_users";
    static const std::string oj_sessions = "oj_sessions";
    static const std::string host = "127.0.0.1";
    static const std::string user = "oj_backend";
    static const std::string passwd = "123456";
    static const std::string db = "oj";
    static const int port = 3306;

    class TimeUtil
    {
    public:
        // 秒级别时间戳
        static const std::string GetTimeStamp_S()
        {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return std::to_string(tv.tv_sec);
        }

        // 毫秒级别时间戳
        static const std::string GetTimeStamp_MS()
        {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return std::to_string((tv.tv_sec * 1000 + tv.tv_usec / 1000));
        }
    };

    class PathUtil
    {
    public:
        static std::string AddSuffix(const std::string &file_name, const std::string &suffix)
        {
            std::string file = temp;
            file += file_name;
            file += suffix;
            return file;
        }

        static std::string Src(const std::string &file_name)
        {
            return AddSuffix(file_name, ".cc");
        }

        static std::string Exe(const std::string &file_name)
        {
            return AddSuffix(file_name, ".exe");
        }

        static std::string Compile_err(const std::string &file_name)
        {
            return AddSuffix(file_name, ".compile_err");
        }

        static std::string Stdin(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stdin");
        }

        static std::string Stdout(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stdout");
        }

        static std::string Stderr(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stderr");
        }
    };

    class FileUtil
    {
    public:
        static bool IsFileExist(const std::string &path_name)
        {
            struct stat st;
            if (stat(path_name.c_str(), &st) == 0)
                return true;
            return false;
        }

        static std::string CreateUniqueFile()
        {
            // 利用 毫秒级时间戳 + 原子递增量 组成唯一文件名
            static std::atomic_int atomic(0);
            atomic++;
            return TimeUtil::GetTimeStamp_MS() + "_" + to_string(atomic);
        }

        static bool WriteFile(const std::string &target_file, const std::string &content)
        {
            std::ofstream out(target_file);
            if (!out.is_open())
            {
                return false;
            }
            out.write(content.c_str(), content.size());
            out.close();
            return true;
        }

        static bool ReadFile(const std::string &target_file, std::string &content, bool keep = false)
        {
            content.clear();
            std::ifstream in(target_file);
            if (!in.is_open())
            {
                return false;
            }
            std::string line;
            while (getline(in, line))
            {
                content += line;
                content += (keep ? "\n" : "");
            }
            in.close();
            return true;
        }

        static std::string GetProjectPath(const std::string& file)
        {
            std::vector<std::string> paths =
            {
                file,
                "../" + file,
                "./oj_server/" + file
            };

            for(const auto& path : paths)
            {
                std::ifstream in(path);
                if(in.good())
                    return path;
            }

            return "";
        }
    };

    class StringUtil
    {
    public:
        static void SplitString(const std::string &str, std::vector<std::string> &target, const std::string &sep)
        {
            boost::split(target, str, boost::is_any_of(sep), boost::algorithm::token_compress_on);
        }
    };

    class MysqlUtil
    {
    public:
        // 执行 INSERT / UPDATE / DELETE，成功返回 true
        static bool Execute(const std::string &sql)
        {
            MYSQL *mysql = Connect();
            if (mysql == nullptr)
            {
                return false;
            }

            if (mysql_query(mysql, sql.c_str()) != 0)
            {
                // LOG(LogLevel::WARNING) << "mysql_query failed, sql : " << sql << std::endl;
                Close(mysql);
                return false;
            }
            Close(mysql);
            return true;
        }

        // 执行 INSERT，成功时返回自增主键（同一连接内 mysql_insert_id）
        static bool ExecuteInsert(const std::string &sql, unsigned long long &insert_id)
        {
            insert_id = 0;
            MYSQL *mysql = Connect();
            if (mysql == nullptr)
                return false;

            if (mysql_query(mysql, sql.c_str()) != 0)
            {
                Close(mysql);
                return false;
            }

            //确保单行插入
            if (mysql_affected_rows(mysql) != 1)
            {
                Close(mysql);
                return false;
            }

            //拿到递增主键
            insert_id = mysql_insert_id(mysql);
            Close(mysql);
            return insert_id != 0;
        }

        // 执行 SELECT，每行调用 handler；handler 返回 false 则提前结束
        static bool Query(const std::string &sql, const std::function<bool(MYSQL_ROW)> &handler)
        {
            MYSQL *mysql = Connect();
            if (mysql == nullptr)
            {
                return false;
            }

            if (mysql_query(mysql, sql.c_str()) != 0)
            {
                Close(mysql);
                return false;
            }

            MYSQL_RES *result = mysql_store_result(mysql);
            if (result == nullptr)
            {
                Close(mysql);
                return false;
            }

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)) != nullptr)
            {
                if (!handler(row))
                {
                    break; // handler 返回 false，提前结束遍历
                }
            }

            mysql_free_result(result);
            Close(mysql);
            return true;
        }

        // 对字符串做 SQL 转义（内部连库 → 转义 → 关库）
        static std::string Escape(const std::string &s)
        {
            MYSQL *mysql = Connect();
            if (!mysql)
                return "";

            std::vector<char> buf(s.size() * 2 + 1, '\0');
            mysql_real_escape_string(mysql, buf.data(), s.c_str(), s.size());

            std::string escaped(buf.data());
            Close(mysql);
            return escaped;
        }

    private:
        // 连接数据库
        static MYSQL *Connect()
        {
            // 初始化数据库
            MYSQL *mysql = mysql_init(nullptr);
            if (mysql == nullptr)
            {
                // LOG(LogLevel::FATAL) << "mysql_init failed";
                return nullptr;
            }

            // 设置编码格式
            mysql_set_character_set(mysql, "utf8");

            // 连接数据库
            if (mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
            {
                // LOG(LogLevel::FATAL) << "mysql_real_connet failed";
                mysql_close(mysql);
                return nullptr;
            }
            //LOG(LogLevel::INFO) << "connect mysql success";
            return mysql;
        }

        static void Close(MYSQL *mysql)
        {
            if (mysql)
                mysql_close(mysql);
        }
    };

    class CryptoUtil
    {
    public:
        // 注册：生成 PHC 编码串，直接写入 oj_users.password
        // plain : 用户输入的明文密码
        static std::string HashPassword(const std::string &plain)
        {
            uint8_t salt[16];  // 16 字节盐
            uint8_t hash[32];  // 32 字节原始哈希
            char encoded[128]; // 足够容纳编码串

            // 从 /dev/urandom 读取随机盐
            std::ifstream urandom("/dev/urandom", std::ios::binary);
            if (!urandom)
                return "";
            urandom.read(reinterpret_cast<char *>(salt), sizeof(salt));
            if (urandom.gcount() != sizeof(salt))
                return "";

            int result = argon2id_hash_encoded(
                3,                           //(时间成本)
                65536,                       //(内存成本)
                4,                           //(并行度[线程数])
                plain.c_str(), plain.size(), //(明文)
                salt, sizeof(salt),
                sizeof(hash),
                encoded, sizeof(encoded));

            if (result != ARGON2_OK)
                return "";
            return std::string(encoded);
        }

        // 登录：与库中 stored 比对
        static bool VerifyPassword(const std::string &plain, const std::string &stored)
        {
            if (stored.empty())
            {
                return false;
            }
            int result = argon2_verify(stored.c_str(), plain.c_str(), plain.size(), Argon2_id);
            return result == ARGON2_OK;
        }

        // session_id：32 字节随机 → 64 位 hex
        static std::string RandomHex(size_t nbytes = 32)
        {
            std::vector<uint8_t> buffer(nbytes);
            std::ifstream urandom("/dev/urandom", std::ios::binary);
            if (!urandom)
                return "";
            urandom.read(reinterpret_cast<char *>(buffer.data()), nbytes);
            if (urandom.gcount() != static_cast<std::streamsize>(nbytes))
                return "";

            std::ostringstream oss;
            oss << std::hex << std::setfill('0');
            for (uint8_t b : buffer)
            {
                oss << std::setw(2) << static_cast<int>(b);
            }
            return oss.str();
        }
    };

    class CookieUtil
    {
    public:
        //从网页发来的req里面提取seesion_id
        static std::string ParseSessionId(const Request &req)
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
    };
}