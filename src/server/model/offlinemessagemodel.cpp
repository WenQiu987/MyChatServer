#include "offlinemessagemodel.hpp"
#include "db.h"

//存储用户的离线消息
void offlineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values (%d, '%s')", userid, msg.c_str());//都要插入就不需要指定字段了

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//删除用户的离线消息
void offlineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);//都要插入就不需要指定字段了

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }

}

//查询用户的离线消息
vector<string> offlineMsgModel::query(int userid)
{

    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    //一个人有多条离线消息，多行，放入vec
    vector<string> vec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                vec.push_back(row[0]);//因为我们上面是select 的message，而不是所有，所以每一行也就只有message了，没有userid
            } 
        }
        mysql_free_result(res);//释放资源
    }
    return vec;
}