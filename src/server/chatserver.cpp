#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <functional>
#include<string>

using namespace std;
using namespace placeholders; //绑定器bind里的 _n, 见p355
using json = nlohmann::json;

//初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    //注册连接函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    //注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));//不对函数名取址会报错，因为setMess..函数的形参指明的是 函数指针 这一可调用对象，而不是函数名

    //设置线程数量
    _server.setThreadNum(4);
}

//启动服务
void ChatServer::start()
{
    _server.start();
}

//上报连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    //客户端断开连接
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

//上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer * buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();//将缓冲区接受到的数据存在字符串里
    //数据的反序列化
    json js = json::parse(buf);

    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());//get<int>()模板函数是将json的messageID转成int型，因为这个ID可能是字符串类型
    //回调消息类型对应的事件处理器，执行相应的业务处理
    msgHandler(conn, js, time);
}
