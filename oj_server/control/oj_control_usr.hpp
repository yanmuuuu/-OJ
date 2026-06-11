#pragma once

#include <iostream>

#include <jsoncpp/json/json.h>

#include "../../comm/log.hpp"
#include "../../comm/util.hpp"
#include "../../comm/httplib.h"
#include "../model/oj_model_sql_usr.hpp"
#include "../view/oj_view.hpp"

namespace ns_control
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;
    using namespace ns_view;
    using namespace httplib;

    class ControlUsr
    {
    public:
        //注册
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

        //退出
        void Logout(const Request &req, std::string &out_json, std::string &set_cookie)
        {
            std::string session_id = CookieUtil::ParseSessionId(req);
            if (!session_id.empty())
                _model_users.DeleteSession(session_id);
            set_cookie = "session_id=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0";
            out_json = BuildAuthJson(0, "已退出");
        }

        //用户信息
        void Me(const Request &req, std::string &out_json)
        {
            _model_users.DeleteExpiredSessions();

            std::string session_id = CookieUtil::ParseSessionId(req);
            if (session_id.empty())
            {
                out_json = BuildAuthJson(0, "已退出");
                return;
            }

            User user;
            if (!_model_users.GetUserBySession(session_id, user))
            {
                _model_users.DeleteSessionIfExpired(session_id);
                out_json = BuildAuthJson(0, "已退出");
                return;
            }

            Json::Value data;
            data["username"] = user.username;
            data["nickname"] = user.nickname;
            out_json = BuildAuthJson(0, "ok", &data);
        }

        //登陆网页
        bool LoginPage(std::string &html)
        {
            _view.LoginExpandHtml(html);
            return true;
        }

        //注册网页
        bool RegisterPage(std::string &html)
        {
            _view.RegisterExpandHtml(html);
            return true;
        }

        //关于网页
        bool AboutPage(std::string &html)
        {
            _view.AboutExpandHtml(html);
            return true;
        }

    private:
        //构建 API 响应
        inline std::string BuildAuthJson(int errcode, const std::string &errmsg, const Json::Value *data = nullptr)
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

        //构造cookie有效时间
        std::string SessionExpireAt()
        {
            time_t expire = time(nullptr) + 7 * 24 * 3600; //天
            struct tm t;
            localtime_r(&expire, &t);
            char buf[20];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
            return std::string(buf);
        }

        //基础检验用户名
        bool ValidateUsernameBasic(const std::string &username, std::string &errmsg, int &errcode)
        {
            if (username.size() < 3 || username.size() > 16)
            {
                errmsg = "用户名长度不合规";
                errcode = 3;
                return false;
            }
            return true;
        }

        //严格检验用户名
        bool ValidateUsernameStrength(const std::string &username, std::string &errmsg, int &errcode)
        {
            if (!ValidateUsernameBasic(username, errmsg, errcode))
                return false;
            if (_model_users.UsernameExists(username))
            {
                errmsg = "该用户名已存在";
                errcode = 1;
                return false;
            }
            return true;
        }

        //基础检验密码
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

        //严格检验密码
        bool ValidatePasswordStrength(const std::string &password, std::string &errmsg, int &errcode)
        {
            if (!ValidatePasswordBasic(password, errmsg, errcode))
                return false;

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
                    errmsg = "密码含有未定义符号(" + std::string(1, c) + ')';
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
        ModelUser _model_users;
        View _view;
    };
}
