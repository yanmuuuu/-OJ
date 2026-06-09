#include "compile_run.hpp"
#include "../comm/httplib.h"

using namespace ns_compile_run;
using namespace httplib;

//在没有写客户端前, 可以使用postman来模拟客户端发送in_json

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Use : " << argv[0] << " port" << std::endl;
        return 1;
    }
    Server svr;

    svr.Post("/compile_and_run", [](const Request& req, Response& resp){
        std::string in_json = req.body;
        if (in_json.empty())
        {
            return;
        }
        std::string out_json;
        Compile_run::Start(in_json, out_json);
        resp.set_content(out_json, "application/json;charset=utf-8");
    });
    svr.listen("0.0.0.0", atoi(argv[1]));
    
    return 0;

    /*
    std::string in_json;
    Json::Value root;
    Json::FastWriter writer;
    root["code"] = R"(#include <iostream>
    int main()
    {
        std::cout << "hello world" << std::endl;
        return 0;
    }
    )";
    root["input"] = "";
    root["cpu_limit"] = 1;
    root["mem_limit"] = 1024 * 30;
    in_json = writer.write(root);
    std::string out_json;
    std::cout << in_json << std::endl;

    Compile_run::Start(in_json, out_json);

    std::cout << out_json << std::endl;
    */
}