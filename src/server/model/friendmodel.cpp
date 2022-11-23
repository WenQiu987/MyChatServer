#include "friendmodel.hpp"
#include "db.h"


// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values (%d, '%d')", userid, friendid);//都要插入就不需要指定字段了

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}


//返回用户的好友列表 
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select u.id, u.name, u.state from user u inner join friend f on f.friendid=u.id where f.userid=%d", userid);

    //一个人有多条离线消息，多行，放入vec
    vector<User> vec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){ //好友很多，所以是while
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.emplace_back(user);//存的是自定义类，比push_back高效一些
            }
            mysql_free_result(res);
        }
    }
    return vec;
}


