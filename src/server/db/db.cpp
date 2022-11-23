#include "db.h"
#include <muduo/base/Logging.h>

// 数据库配置信息，全局静态变量，仅在当前文件可用
static string server = "127.0.0.1";
static string user = "root";
static string password = "11235813";
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
        mysql_close(_conn);
}

// 连接数据库
bool MySQL::connect()
{
    if(!_conn){ LOG_INFO <<"_conn是空！";}
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0); // string类的成员函数c_str()返回字符串首地址
    if (p != nullptr)
    {
        // C和C++代码默认的编码字符是ASCII码，这里设置字段name为gbk编码，如果不设置，则从mysql上传来的中文会显示成问号?
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO <<"connect mysql failed!";
    }

    return p;//p只是判断是否连接成功，真正连接的还是 _conn指针.其实对于成功的连接，返回值p与第1个参数的值即_conn相同
}

// 更新操作,可以传delete,update,insert相关的sql语句
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))//mysql_query是查询成功返回0，否则非0
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!";
        return false;
    }
    return true;
}

// 查询操作, 主要针对select
MYSQL_RES* MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);//返回查询结果
}

//获取连接
MYSQL* MySQL::getConnection()
{
    return _conn;
}