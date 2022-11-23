#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group& group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());//群号是创建后才有的

    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 用户加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser(groupid, userid, grouprole) values (%d, %d, '%s')", groupid, userid, role.c_str());//注意枚举enum在sql中当做字符串

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}


// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a inner join groupuser g on g.groupid=a.id where g.userid=%d", userid);

    // 一个人可能加入多个群组
    vector<Group> groupVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);//因为我们上面是select 的message，而不是所有，所以每一行也就只有message了，没有userid
            } 
        }
        mysql_free_result(res);//释放资源
    }
    
    //还要查这个组其它组员的信息
    for(auto &group : groupVec){
        sprintf(sql, "select u.id, u.name, u.state, g.grouprole from user u inner join groupuser g on g.userid=u.id where g.groupid=%d", group.getId());

        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);//因为getUsers那返回的是引用，所以获取vector<GroupUser>对象再push_back元素进去也会改变原对象
            }
            mysql_free_result(res);
        }
    }
    // 最后再返回结果
    return groupVec;
}


//根据指定的groupid查询群组用户id列表，除userid自己，主要用于群聊业务给群组其它成员群发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid !=%d", groupid, userid);


    vector<int> idVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}