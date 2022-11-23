#include "usermodel.hpp"
#include "db.h"
#include<iostream>
using namespace std;

// User表的增加方法
bool UserModel::insert(User& user)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values ('%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    // 插入数据mysql.update(sql)
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){ //判读是否插入成功：因为可能name已经存在，此时就会添加失败
            // 获取插入数据的主键id，用mysql.h的方法mysql_insert_id()
            //因为主键是自增键，我们插入sql语句那会没有ID，插入后就自增生成了，然后获取它，再将其赋值给user的成员变量id，否则user.id一直都是默认值1
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false; 
}

// 根据用户号码即id, 查询用户信息
User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        // 查成功了
        if(res != nullptr){
            MYSQL_ROW row = mysql_fetch_row(res);//只要获取一行，即所查id的那一行所有信息,MYSQL_ROW是个字符串数组类型，是char**别名
            if(row != nullptr){
                User user;
                user.setId(atoi(row[0]));//row[0]第一个字符串，此时的id是char*类型，所以
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);//释放指针，防止内存泄露
                return user;
            }
        }
    }
    return User();//没找到就返回一个User临时对象，id默认是-1
}

// 更新用户状态信息
bool UserModel::updateState(User user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state= '%s' where id =%d", user.getState().c_str(), user.getId());//写进字段state是单引号，且%s是填char*所以要转换

    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 重置用户的状态信息
void UserModel::resetState()
{
    char sql[1024] = "update user set state= 'offline' where state ='online'";

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}