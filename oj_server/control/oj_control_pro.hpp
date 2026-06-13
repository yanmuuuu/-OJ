#include "../model/oj_model_sql_pro.hpp"
#include "../model/oj_model_sql_usr.hpp"
#include "../view/oj_view.hpp"
namespace ns_control
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;
    using namespace ns_view;
    using namespace httplib;

    class ControlProgress
    {
    public:
        void MyProgress(const Request &req, std::string &out_json)
        {
            _model_users.DeleteExpiredSessions();

            std::string session_id = CookieUtil::ParseSessionId(req);
            if (session_id.empty())
            {
                out_json = BuildAuthJson(1, "请先登录");
                return;
            }

            User user;
            if (!_model_users.GetUserBySession(session_id, user))
            {
                _model_users.DeleteSessionIfExpired(session_id);
                out_json = BuildAuthJson(1, "请先登录");
                return;
            }

            std::string sql = "SELECT question_number, status, updated_at FROM " 
                            + oj_user_progress
                            + " WHERE user_id = " 
                            + std::to_string(user.id);

            std::vector<UserProgress> rows;
            if (!_model_progress.QueryMysqlSelect(sql, rows))
            {
                out_json = BuildAuthJson(4, "系统繁忙，请稍后重试");
                return;
            }

            sql = "SELECT p.question_number, p.status, p.updated_at, q.title, q.star FROM "
                + oj_user_progress + " p JOIN " + oj_questions
                + " q ON p.question_number = q.number WHERE p.user_id = "
                + std::to_string(user.id) + " ORDER BY p.updated_at DESC";
            
            std::vector<UserProgressDetail> details;
            if (!_model_progress.QueryMysqlSelectDetail(sql, details))
            {
                out_json = BuildAuthJson(4, "系统繁忙，请稍后重试");
                return;
            }

            Json::Value data;
            Json::Value map(Json::objectValue);
            Json::Value solved_arr(Json::arrayValue);
            Json::Value attempted_arr(Json::arrayValue);
            int solved_count = 0;
            int attempted_count = 0;
            for (const auto &p : rows)
            {
                std::string status_str = (p.status == ProgressStatus::Solved) ? "solved" : "attempted";
                map[p.question_number] = status_str;
            }
            
            for (const auto &d : details)
            {
                Json::Value item;
                item["number"] = d.question_number;       
                item["title"] = d.title;
                item["star"] = d.star;
                item["updated_at"] = d.updated_at;
                if (d.status == ProgressStatus::Solved)
                {
                    solved_arr.append(item);
                    solved_count++;
                }
                else
                {
                    attempted_arr.append(item);
                    attempted_count++;
                }
            }
            data["map"] = map;
            data["solved"] = solved_arr;
            data["attempted"] = attempted_arr;
            Json::Value stats;
            stats["solved"] = solved_count;
            stats["attempted"] = attempted_count;
            data["stats"] = stats;
            out_json = BuildAuthJson(0, "ok", &data);
        }

        bool MyProgressPage(const Request &req, std::string &html)
        {
            (void)req;
            _view.MyProgressExpandHtml(html);
            return true;
        }

    private:
        static std::string BuildAuthJson(int errcode, const std::string &errmsg, const Json::Value *data = nullptr)
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

        ModelUser _model_users;
        ModelProgress _model_progress;
        View _view;
    };
}