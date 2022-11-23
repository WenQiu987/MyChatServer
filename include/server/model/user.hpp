#ifndef USER_H
#define USER_H
#include <string>
using namespace std;

//匹配User表的ORM类（对象映射关系类）
class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline") : id(id), name(name), password(pwd), state(state) {}

    void setId(int id) { this->id = id; } //一定要加this，因为形参和实参名字一样
    void setName(string name) { this->name = name; }
    void setPwd(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPwd() { return this->password; }
    string getState() { return this->state; }

protected:
    int id;
    string name;
    string password;
    string state;
};

#endif