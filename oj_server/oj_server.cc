#include <iostream>

#include "../comm/httplib.h"
#include "control/oj_control.hpp"

using namespace httplib;
using namespace ns_control;

Control *ctl_ptr = nullptr;

void Recover(int sig)
{
    ctl_ptr->RecoveryMachine();
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Use : " << argv[0] << " port" << std::endl;
        return 1;
    }

    signal(SIGINT, Recover);

    Server svr;
    Control ctl;
    ctl_ptr = &ctl;

    svr.Get("/all_questions", [&ctl](const Request &req, Response &resp) {
        std::string html;
        ctl.AllQuestions(html);
        resp.set_content(html, "text/html;charset=utf-8"); 
    });

    svr.Get(R"(/question/(\d+))", [&ctl](const Request &req, Response &resp) {
        std::string number = req.matches[1];
        std::string html;
        ctl.OneQuestion(number, html);
        resp.set_content(html, "text/html;charset=utf-8"); 
    });

    svr.Post(R"(/judge/(\d+))", [&ctl](const Request &req, Response &resp) {
        std::string number = req.matches[1];
        std::string result_json;
        ctl.Judge(number, req.body, result_json);
        resp.set_content(result_json, "application/json;charset=utf-8"); 
    });

    svr.Post("/api/login", [&ctl](const Request &req, Response &resp) {
        std::string out_json, set_cookie;
        ctl.Login(req.body, out_json, set_cookie);
        resp.set_content(out_json, "application/json;charset=utf-8");
        if (!set_cookie.empty())
            resp.set_header("Set-Cookie", set_cookie); 
    });

    svr.Post("/api/register", [&ctl](const Request &req, Response &resp) {
        std::string out_json;
        ctl.Register(req.body, out_json);
        resp.set_content(out_json, "application/json;charset=utf-8");
    });

    svr.Post("/api/logout", [&ctl](const Request &req, Response &resp) {
        std::string out_json, set_cookie;
        ctl.Logout(req, out_json, set_cookie);
        resp.set_content(out_json, "application/json;charset=utf-8");
        resp.set_header("Set-Cookie", set_cookie);
    });

    svr.Get("/api/me", [&ctl](const Request &req, Response &resp) {
        std::string out_json;
        ctl.Me(req, out_json);
        resp.set_content(out_json, "application/json;charset=utf-8");
    });

    svr.Get("/login", [&ctl](const Request &req, Response &resp) {
        std::string html;
        ctl.LoginPage(html);
        resp.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get("/register", [&ctl](const Request &req, Response &resp) {
        std::string html;
        ctl.RegisterPage(html);
        resp.set_content(html, "text/html;charset=utf-8");
    });

    svr.set_base_dir("./wwwroot");
    svr.listen("0.0.0.0", atoi(argv[1]));

    return 0;
}