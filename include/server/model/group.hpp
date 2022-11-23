#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;

// GroupUser表，AllGroup表的ORM类（对象映射关系类）
class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    void setId(int id) { this->id = id; }
    //群组的名字和描述
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getId() { return this->id = id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc;}
    vector<GroupUser>& getUsers() { return this->users; }


private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;// 组员，包括的user的所有属性和一个角色role

};


#endif