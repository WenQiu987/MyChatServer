#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"


// 记录当前系统登录的用户信息，id name friends group等
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 控制主菜单页面程序
static bool isMainMenuRunning = false;  // 可以让mainMenu在用户退出结束,否则一直阻塞在mainMenu()函数
//全局静态相比全局变量，它只在本文件共享，其他文件不能，而且其他文件可以命名同名的变量了

// 显示当前登录成功用户的基本信息
void showCurrentUserData();
// 接收消息的线程执行的函数
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端程序实现， main线程用作发送线程，子线程用作接收线程
int main(int argc, char** argv)
{
    if(argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息，ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client 和 server 进行连接
    if(-1 == connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // main线程用于接收用户输入，负责发送数据
    for(;;)             // 死循环，用户没有执行退出就一直保持这个页面
    {
        // 显示首页面菜单，登录、注册、退出
        cout << "=============================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "=============================" << endl;
        cout << "choice: ";
        int choice = 0;
        
        cin >> choice; // 因为我们输入一个数字，再回车，其实就是输入了数字和"\n"，但是cin只从缓冲区读取了数字，回车还残留着，如果后面再输入了字符串
        cin.get();     // 那一读到回车符"\n"就不读了，这样使得后面的字符串读不到。所以需要接上cin.get()把缓冲区残留的回车符读走
        
        switch(choice)
        {
            case 1:  // login业务
            {
                int id = 0;
                char pwd[50] = {0};
                cout << "userid:";
                cin >> id;
                cin.get(); //读走缓冲区的回车
                cout << "userpassword:";
                cin.getline(pwd, 50); // 用cin.getline()，因为cin>>遇到空格就当做两个字符串了，用户名字可能有空格，比如zhang san

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0); // 发送给服务器，成功则返回发送成功的字节数，否则返回-1
                if(len == -1)
                {
                    cerr << "send login msg error: " << request << endl;
                }
                else
                {
                    char buffer[1024] = {0};
                    len =  recv(clientfd, buffer, 1024, 0);
                    if(len == -1)
                    {
                        cerr << "recv login response error" << endl;
                    }
                    else
                    {
                        // 接收到了服务器的数据也得判断是否登录成功，对方发来errno不为0，说明登录出错
                        json responsejs = json::parse(buffer);
                        if(responsejs["errno"].get<int>() != 0)  // 登录失败
                        {
                            cerr << responsejs["errmsg"] << endl;
                        }
                        else                                     // 登录成功
                        {
                            // 记录当前用户的id和name
                            g_currentUser.setId(responsejs["id"].get<int>()); // 定义的全局变量g_currentUser记录用户信息
                            g_currentUser.setName(responsejs["name"]);

                            //记录当前用户的好友列表信息
                            if(responsejs.contains("friends"))
                            {
                                // 初始化
                                g_currentUserFriendList.clear();

                                vector<string> vec = responsejs["friends"];
                                for(auto &str : vec)
                                {
                                    json js = json::parse(str);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }

                            // 记录当前用户的群组列表信息
                            if(responsejs.contains("groups"))
                            {
                                // 初始化
                                g_currentUserGroupList.clear();

                                vector<string> vec1 = responsejs["groups"];
                                for(string &groupstr : vec1)
                                {
                                    json grpjs = json::parse(groupstr);
                                    Group group;
                                    group.setId(grpjs["id"].get<int>());
                                    group.setName(grpjs["groupname"]);
                                    group.setDesc(grpjs["groupdesc"]);

                                    vector<string> vec2 = grpjs["users"];
                                    for(string &userstr : vec2)
                                    {
                                        GroupUser user;
                                        json js = json::parse(userstr);
                                        user.setId(js["id"].get<int>());
                                        user.setName(js["name"]);
                                        user.setState(js["state"]);
                                        user.setRole(js["role"]);
                                        group.getUsers().push_back(user);
                                    }

                                    g_currentUserGroupList.push_back(group);
                                }
                            }

                            // 显示登陆用户的基本信息
                            showCurrentUserData();

                            // 显示当前用户的离线消息:   包括  个人聊天信息或者群组消息
                            if(responsejs.contains("offlinemsg"))
                            {
                                vector<string> vec = responsejs["offinemsg"];
                                for(string &str : vec)
                                {
                                    json js = json::parse(str);

                                    //不是个人消息就是群聊消息
                                    if (ONE_CHAT_MSG == js["msgid"].get<int>())
                                    {
                                        cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                             << " said: " << js["msg"].get<string>() << endl;
                                    }
                                    else
                                    {
                                        cout << "群消息[" << js["groupid"] << "]" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                             << " said: " << js["msg"].get<string>() << endl;
                                    }
                                }
                            }

                            // 登录成功，启动接收线程负责接收数据, 一个用户登录该线程只会启动一次
                            static int threadNumber = 0;
                            if(threadNumber == 0){
                                std::thread readTask(readTaskHandler, clientfd);
                                readTask.detach();
                                threadNumber++;
                            }
                            

                            // 进入聊天主菜单页面
                            isMainMenuRunning = true;
                            mainMenu(clientfd);
                        }

                    }
                }
            }
            break;
            case 2:  // register注册业务
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username: ";
                cin.getline(name, 50);
                cout << "userpassword:  ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);  
                if(len == -1)
                {
                    cerr << "send reg msg error: " << request << endl;
                }
                else
                {
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);  //接收服务端的响应，保存在buffer里 —— — —阻塞等待
                    if(len == -1)
                    {
                        cerr << "recv reg response error" << endl;
                    }
                    else
                    {
                        json responsejs = json::parse(buffer);
                        // 注意还有个判断，根据服务器的响应，errno为1，注册失败；为0，注册成功
                        if(responsejs["errno"].get<int>() != 0) // 注册失败
                        {
                            cerr << name <<" is already exist, register error! " << endl;
                        }
                        else // 注册成功
                        {
                            cout << name << " register success, userid is " << responsejs["id"] << ", do not forget it!" << endl;
                        }
                    }
                }
            }
            break;
            case 3:  // quit业务
                close(clientfd);
                exit(0);
            default:
                cerr << "invalid input!" << endl;
                break;
        }
    }
    return 0;
}

// 接收信息的线程执行的函数
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);  // 如果用户退出了，会阻塞在这
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        } 
        if(GROUP_CHAT_MSG == msgtype){
            cout << "群消息[" << js["groupid"] << "]" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
        }
    }

}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void logout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天, 格式chat:friendid:message"},
    {"addfriend", "添加好友, 格式addfriend:friendid"},
    {"creategroup", "创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组, 格式addgroup:groupid"},
    {"groupchat", "群聊, 格式groupchat:groupid:message"},
    {"logout", "注销, 格式logout"}};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout}};

// 主聊天页面
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");
        if(-1 == idx)
        {
            command = commandbuf; //1.没找到那就是help和logout两个命令；2.或者用户是随便输入的，那后面也会find，找不到那就是输错了
        }
        else{
            command = commandbuf.substr(0, idx); //不是区间，string.substr(startIdx,length)
        }
        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的事件回调,两个：的情况就在相应函数里再写find(:)即可
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 其实这里length比剩下字符数还大，这种情况就返回剩下的所有字符，所有不必精算-idx-1
                                                                                    
    }



}

// "help" command handler
void help(int, string)
{
    cout << "show command list >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}
// "chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    // string.c_str() 转 const char* 时, 会在字符串末尾自动补“\0”，而strlen是不算终止符\0的，所以要+1,也可以buffer.size()+1
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0); //服务器那边收到了就解析，然后发给对应的好友
    if (-1 == len)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }
}

// "addfriend" command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}

// "creategroup" command handler
void creategroup(int clientfd, string str)
{
    int idx = str.find(":"); // groupname:groupdesc
    if(-1 == idx){
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len){
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

// "addgroup" command handler
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}

// "groupchat" command handler
void groupchat(int clientfd, string str)
{
    int idx = str.find(":"); // groupid:message
    if (-1 == idx)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    // string.c_str() 转 const char* 时, 会在字符串末尾自动补“\0”，而strlen是不算终止符\0的，所以要+1,也可以buffer.size()+1
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0); //服务器那边收到了就解析，然后发给对应的好友
    if (-1 == len)
    {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }

}

// "loginout" command handler
void logout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len){
        cerr << "send logout msg error -> " << buffer << endl;
    }else{
        isMainMenuRunning = false; // 退出后，mainMenu()就不执行while循环了
    }

}


// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);  // 获取本地时间，存在tm类结构体
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",                                            // 02d% 是说输出整数不足2位就左侧补0
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}