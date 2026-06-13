#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "../../comm/log.hpp"
#include "../../comm/util.hpp"
#include "../../third_party/include/mysql.h"

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_util;

    enum class ProgressStatus
    {
        Attempted,
        Solved
    };

    struct UserProgress
    {
        std::string question_number;
        ProgressStatus status;
        std::string updated_at;
    };

    struct UserProgressDetail : public UserProgress
    {
        std::string title;
        std::string star;
    };

    class ModelProgress
    {
    public:
        bool QueryMysqlSelect(const std::string &sql, std::vector<UserProgress> &out)
        {
            return MysqlUtil::Query(sql, [&out](MYSQL_ROW row) -> bool {
                UserProgress up;
                up.question_number = row[0] ? row[0] : "";
                if (row[1] && std::string(row[1]) == "solved")
                    up.status = ProgressStatus::Solved;
                else
                    up.status = ProgressStatus::Attempted;
                up.updated_at = row[2] ? row[2] : "";
                out.push_back(up);
                return true;
            });
        }

        bool QueryMysqlSelectDetail(const std::string &sql, std::vector<UserProgressDetail> &out)
        {
            return MysqlUtil::Query(sql, [&out](MYSQL_ROW row) -> bool {
                UserProgressDetail upd;
                upd.question_number = row[0] ? row[0] : "";
                if (row[1] && std::string(row[1]) == "solved")
                    upd.status = ProgressStatus::Solved;
                else
                    upd.status = ProgressStatus::Attempted;
                upd.updated_at = row[2] ? row[2] : "";
                upd.title = row[3] ? row[3] : "";
                upd.star = row[4] ? row[4] : "";
                out.push_back(upd);
                return true;
            });
        }

        bool QueryMysqlOther(const std::string &sql)
        {
            return MysqlUtil::Execute(sql);
        }
    };
}
