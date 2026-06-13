#pragma once

#include <string>

#include "../../comm/httplib.h"
#include "oj_control_usr.hpp"
#include "oj_control_que.hpp"
#include "oj_control_pro.hpp"

namespace ns_control
{
    using namespace httplib;

    class Control
    {
    public:
        void Register(const std::string &in_json, std::string &out_json)
        {
            _usr.Register(in_json, out_json);
        }

        void Login(const std::string &in_json, std::string &out_json, std::string &set_cookie)
        {
            _usr.Login(in_json, out_json, set_cookie);
        }

        void Logout(const Request &req, std::string &out_json, std::string &set_cookie)
        {
            _usr.Logout(req, out_json, set_cookie);
        }

        void Me(const Request &req, std::string &out_json)
        {
            _usr.Me(req, out_json);
        }

        bool LoginPage(std::string &html)
        {
            return _usr.LoginPage(html);
        }

        bool RegisterPage(std::string &html)
        {
            return _usr.RegisterPage(html);
        }

        bool AboutPage(std::string &html)
        {
            return _usr.AboutPage(html);
        }

        bool AllQuestions(std::string &html)
        {
            return _que.AllQuestions(html);
        }

        bool OneQuestion(const std::string &number, std::string &html)
        {
            return _que.OneQuestion(number, html);
        }

        void Judge(const Request &req, const std::string &number, const std::string &in_json, std::string &out_json)
        {
            _que.Judge(req, number, in_json, out_json);
        }

        void SubmitQuestion(const Request &req, const std::string &in_json, std::string &out_json)
        {
            _que.SubmitQuestion(req, in_json, out_json);
        }

        bool SubmitQuestionPage(std::string &html)
        {
            return _que.SubmitQuestionPage(html);
        }

        void RecoveryMachine()
        {
            _que.RecoveryMachine();
        }

        void Run(const std::string& number, const std::string &in_json, std::string &out_json)
        {
            _que.Run(number, in_json, out_json);
        }

        void MyProgress(const Request &req, std::string &out_json)
        {
            _pro.MyProgress(req, out_json);
        }

        bool MyProgressPage(const Request &req, std::string &html)
        {
            return _pro.MyProgressPage(req, html);
        }

    private:
        ControlUsr _usr;
        ControlQue _que;
        ControlProgress _pro;
    };
}
