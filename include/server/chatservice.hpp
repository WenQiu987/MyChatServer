#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <string>
#include <mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "redis.hpp"
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
using json = nlohmann::json;

// 处理消息事件回调方法的类型名
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json& js, Timestamp time)>;//using取别名，消息处理器

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();
    
    // 登录
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 注销 退出
    void logout(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 注册
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);
    // 服务器异常，业务重置方法
    void reset();
    // 获取消息messageID对应的处理器—— ——函数指针
    MsgHandler getHandler(int msgid);

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);



private:
    ChatService();

    // 存储消息id和其对应的业务处理方法。key是整型的消息id，value是对应操作函数
    unordered_map<int, MsgHandler> _msghandlerMap;

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    // 数据操作对象
    UserModel _userModel;
    offlineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    // Redis操作对象
    Redis _redis;
};


#endif