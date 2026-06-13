#pragma once

#include <iostream>

#include <ctemplate/template.h>

#include "../model/oj_model_sql_que.hpp"

namespace ns_view
{
    using namespace ns_model;
    class View
    {
    public:
        View()
        {
        }

        ~View()
        {
        }

        void AllExpandHtml(const std::vector<Question> &questions, std::string &html)
        {
            // 创建根字典
            ctemplate::TemplateDictionary dict("all_questions");
            for (const auto &q : questions)
            {
                // 为每个题目创建一个子字典，并添加到 "QUESTIONS" 这个Section中
                ctemplate::TemplateDictionary *sub_dict = dict.AddSectionDictionary("QUESTIONS");
                // 使用与模板匹配的大写变量名来设置值
                sub_dict->SetValue("NUMBER", q.number);
                sub_dict->SetValue("TITLE", q.title);
                sub_dict->SetValue("STAR", q.star);
                sub_dict->SetValue("CPU_LIMIT", std::to_string(q.cpu_limit));
                sub_dict->SetValue("MEM_LIMIT", std::to_string(q.mem_limit));
                sub_dict->SetValue("AUTHOR_NAME", q.author_name.empty() ? "官方" : q.author_name);
            }

            // 加载模板
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(FileUtil::GetProjectPath("./template/all_questions.html"), ctemplate::DO_NOT_STRIP);
            if (!tpl)
            {
                html = "模板文件加载失败: all_questions.html";
                return;
            }

            // 展开模板，将结果填充到html字符串中
            tpl->Expand(&html, &dict);
        }

        void OneExpandHtml(const Question &question, std::string &html)
        {
            ctemplate::TemplateDictionary dict("one_question");
            dict.SetValue("NUMBER", question.number);
            dict.SetValue("TITLE", question.title);
            dict.SetValue("STAR", question.star);
            dict.SetValue("CPU_LIMIT", std::to_string(question.cpu_limit));
            dict.SetValue("MEM_LIMIT", std::to_string(question.mem_limit));
            dict.SetValue("DESC", question.desc);
            dict.SetValue("AUTHOR_NAME", question.author_name.empty() ? "官方" : question.author_name);

            // 重要：只提供 head.cpp 内容（函数签名/框架），让用户填写函数体
            dict.SetValue("CODE", question.head); // 而不是 head + tail
            dict.SetValue("RUN_CASE", question.run_case);

            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(FileUtil::GetProjectPath("./template/one_question.html"), ctemplate::DO_NOT_STRIP);
            if (!tpl)
            {
                html = "模板文件加载失败: one_question.html";
                return;
            }
            tpl->Expand(&html, &dict);
        }

        void LoginExpandHtml(std::string &html)
        {
            ctemplate::TemplateDictionary dict("login");
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(FileUtil::GetProjectPath("./template/login.html"), ctemplate::DO_NOT_STRIP);
            if (!tpl)
            {
                html = "模板文件加载失败: login.html";
                return;
            }
            tpl->Expand(&html, &dict);
        }

        void RegisterExpandHtml(std::string &html)
        {
            ctemplate::TemplateDictionary dict("register");
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(FileUtil::GetProjectPath("./template/register.html"), ctemplate::DO_NOT_STRIP);
            if (!tpl)
            {
                html = "模板文件加载失败: register.html";
                return;
            }
            tpl->Expand(&html, &dict);
        }

        void SubmitQuestionExpandHtml(std::string &html)
        {
            ctemplate::TemplateDictionary dict("submit_question");
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(FileUtil::GetProjectPath("./template/submit_question.html"), ctemplate::DO_NOT_STRIP);
            if (!tpl)
            {
                html = "模板文件加载失败: submit_question.html";
                return;
            }
            tpl->Expand(&html, &dict);
        }

        void AboutExpandHtml(std::string &html)
        {
            ctemplate::TemplateDictionary dict("about");
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(FileUtil::GetProjectPath("./template/about.html"), ctemplate::DO_NOT_STRIP);
            if (!tpl)
            {
                html = "模板文件加载失败: about.html";
                return;
            }
            tpl->Expand(&html, &dict);
        }

        void MyProgressExpandHtml(std::string &html)
        {
            ctemplate::TemplateDictionary dict("my_progress");
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(FileUtil::GetProjectPath("./template/my_progress.html"), ctemplate::DO_NOT_STRIP);
            if (!tpl)
            {
                html = "模板文件加载失败: my_progress.html";
                return;
            }
            tpl->Expand(&html, &dict);
        }
    };
}