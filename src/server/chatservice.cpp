#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h> //日志库
#include <string>
#include <vector>
using namespace std;
using namespace muduo;

//获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service; //局部静态变量，线程安全的单例模式
    return &service;
}

//构造函数，往unordered_map中注册消息对应的函数操作Handler
ChatService::ChatService()
{
    // 用户基本业务_回调函数注册
    _msghandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)}); //这里用函数指针作为可调用对象，其实函数名也可以,而且取地址时作用域也得紧跟在变量前面，本来是&login
    _msghandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});
    _msghandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msghandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msghandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组相关业务_回调函数注册
    _msghandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msghandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msghandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});


    // 连接redis服务器
    if(_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    _userModel.resetState();

}


// 获取消息messageID对应的处理器—— ——回调函数
MsgHandler ChatService::getHandler(int msgid)
{
    //错误日志，msgid没找到对应处理回调函数
    auto it = _msghandlerMap.find(msgid);
    if (it == _msghandlerMap.end())
    {
        //返回一个默认的处理器，空操作,只输出日志
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid: " << msgid << "can not find handler!"; // muduo提供的
        };
    }
    else
    {
        return _msghandlerMap[msgid]; //找到了就返回对应的回调函数
    }
}

//登录业务 id password
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string pwd = js["password"];

    User user = _userModel.query(id); // id相当于QQ号，name就是昵称。根据QQ号来返回用户信息，再去匹配输入密码

    //先处理用户名id错误的情况
    if (user.getId() == -1)
    {
        // id错误，该用户不存在，返回了默认值-1
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "The user does not exist!";
        conn->send(response.dump());// conn是智能指针类对象，调用send()函数，要求传入参数为string类，所以得dump()
    }
    else if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            //该用户已经在线，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "This account is using, input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功
            // 记录用户连接信息,注意加锁，防止多线程冲突
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn}); //临界区代码
            }

            // 用户登录成功，向redis订阅channel(id)
            _redis.subscribe(id);

            // 更新下用户的登录信息
            user.setState("online");
            _userModel.updateState(user); //数据库记得更新

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询是否有离线信息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty()){
                response["offlinemsg"] = vec;
                //读取该用户离线消息后，把该用户的离线消息都清空
                _offlineMsgModel.remove(id);
            }

            // 查询用户的好友信息
            //response["friend"] = _friendModel.query(user.getId());不能直接放置,因为容器内元素类型是自定义的user
            vector<User> userVec = _friendModel.query(id);//登录成功那id就是user.getid()
            if(!userVec.empty())
            {
                vector<string> vec1;
                for(auto user : userVec){
                    json js1;
                    js1["id"] = user.getId();
                    js1["name"] = user.getName();
                    js1["state"] = user.getState();
                    vec1.push_back(js1.dump());//得dump成字符串
                }
                response["friends"] = vec1;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if(!groupuserVec.empty())
            {
                vector<string> groupV; // 将群组信息查询出来后存到这里
                for(Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();

                    vector<string> userV;
                    for(GroupUser &gUser : group.getUsers())
                    {
                        json js;
                        js["id"] = gUser.getId();
                        js["name"] = gUser.getName();
                        js["state"] = gUser.getState();
                        js["role"] = gUser.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }

    //只剩密码输错的情况
    else
    {
        //用户存在，但是密码错了
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 3;
        response["errmsg"] = "The password is incorrect, please enter it again";
        conn->send(response.dump());
    }
}

//注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);

    bool state = _userModel.insert(user);

    //注册成功与否的响应
    if (state)
    {
        //注册成功,对客户端的反馈
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败————说明昵称已经存在了, 注册这边没有"errmsg"，在客户端代码实现了
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理注销业务————用户退出
void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    //更新用户的状态信息
    User user(userid, "", "", "offline"); // 名字和密码传啥都行，updateState只根据userid来更新用户state
    _userModel.updateState(user);

}


// 处理客户端异常退出  — — — — — —比如Ctrl + C
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 删除哈希表中用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    } // 互斥锁的作用域

    // 用户下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    // 更新数据库中对应id的用户的状态
    // 防止conn在哈希表中没有，使得user的id为默认值,不判断也行，到时候mysql会提示没有此用户
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int to_id = js["toid"].get<int>();    //要发给的人的id

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(to_id);
        if (it != _userConnMap.end())
        {
            // to_id用户在线，服务器主动转发消息给to_id用户
            it->second->send(js.dump());//智能指针conn调用函数send，其形参为字符串类型
            return;
        }

    }
    // 集群服务器中，A给B发消息，B可能在线但是没在A这台服务器上，所以得查数据库
    User user = _userModel.query(to_id);
    if(user.getState() == "online"){
        _redis.publish(to_id, js.dump()); // 发布消息给to_id通道
        return;
    }

    // to_id不在线，存储离线消息
    _offlineMsgModel.insert(to_id, js.dump()); 
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    // js是收到的消息，且指明了哪个用户发的，在哪个群组发的
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    // 要操作_userConnMap,所以要加互斥锁
    lock_guard<mutex> lock(_connMutex);
    for(int id : useridVec){
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end()){
            // it存在说明用户在线，则将消息发送给该用户
            it->second->send(js.dump());
        }
        else
        {
            // 也可是不在这台服务器上，所以查询数据库
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id, js.dump()); // 发布消息给to_id通道
                return;
            }
            else{
                // 不在线，将消息存为离线消息
                _offlineMsgModel.insert(id, js.dump());
            } 
        }
    }
}

// 添加好友业务 
void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid, friendid);
    /* 
    
    其实这里可以补充下添加成功的消息，就像登录和注册那边一样
    还有就是能不能让对方选择同意还是不同意
    
    */

}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();//不是群组id是用户id，群号要等创建好了才能获得
    string name = js["groupname"];
    string desc = js["gorupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if(_groupModel.createGroup(group)){
        std::cout<<"创建成功"<<endl;
        //创建成功则存下群主信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}



// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end()){
        it->second->send(msg); // 这里是服务器给登录在自己上的那些客户端发消息，该函数由redis那边调用。即redis发现被订阅的channel有消息了，就让服务器给对应的客户端发消息
        return;
    }

    // 若不在线，存储离线消息 ————这里不需要再查数据库了，原因见上个注释
    _offlineMsgModel.insert(userid, msg);
    
}