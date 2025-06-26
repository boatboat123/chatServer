#include "db.h"
#include <mymuduo/Logger.h>

static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}
// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        mysql_query(_conn, "set names gbk");
        LOG_INFO("connect mysql success!");
    }
    else
    {
        LOG_INFO("connect mysql fail!");
    }
    return p;
}
// 更新操作
// bool MySQL::update(string sql)
// {
//     if (mysql_query(_conn, sql.c_str()))
//     {
//         LOG_INFO("更新失败");
//         return false;
//     }
//     LOG_INFO("更新成功");
//     return true;
// }

bool MySQL::update(string sql) {
    // 检查连接是否有效
    if (_conn == nullptr) {
        LOG_ERROR("MySQL 连接无效，请先调用 connect()");
        return false;
    }

    // 执行 SQL
    if (mysql_query(_conn, sql.c_str()) != 0) {
        LOG_ERROR("SQL 执行失败: %s\n错误信息: %s", 
                 sql.c_str(), mysql_error(_conn));  // 输出 SQL 和错误详情
        return false;
    }

    LOG_INFO("SQL 执行成功: %s", sql.c_str());  // 记录成功的 SQL
    return true;
}
// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO("查询失败");
        return nullptr;
    }
    return mysql_use_result(_conn);
}

MYSQL* MySQL::getConnection()
{
    return _conn;
}
