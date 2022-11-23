#include "json.hpp"
using json = nlohmann::json;
#include<iostream>
#include<map>
#include<vector>
#include<string>


using namespace std;

//json序列化演示
string func1(){
    json js;
    //就像map容器一样，key - value
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, it's my first cpp program";
    string Str = js.dump();
    return Str;
}

string func2(){
    json js;
    js["id"] = {1, 2, 3, 4, 5};
    js["name"] = "zhang san";
    //添加对象，key可以是二维数组
    js["msg"]["liu shuo"] = "hello A";
    js["msg"]["zhang san"] = "hello B";
    //下面等同于上面两行
    //js["msg"] = {{"liu shuo", "hello A"}, {"zhang san", "hello B"}};//用两个json对象给js的msg赋值
    return js.dump();
}

string func3(){
    json js;
    //序列化vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;

    //序列化map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;
    return js.dump();
}

int main(){
    json js = json::parse(func3());
    vector<int> nums = js["list"];//因为js里面的list的值是vector形式数据
    map<int, string>mp = js["path"];
    for(auto& a : nums){
        cout<<a<<endl;
    }
    //下标输出map的value
    for(int i = 0; i < mp.size(); ++i){
        cout<<mp[i]<<endl;
    }
    //范围for循环输出map的键值对
    for(auto &p : mp){
        cout<<p.first<<" "<<p.second<<endl;
    }


    return 0;
}

