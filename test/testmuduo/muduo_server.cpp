/*
muduo网络库给用户提供了两个主要的类：

TcpSrever : 用于编写服务器程序
TcpClient  :  用于编写客户端程序

也就是将epoll + 线程池封装好了，可以把网络I/O的代码和业务代码区分开,只需关注：
==用户的连接和断开==， ==用户的可读可写事件==
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders; //绑定器里的 _n

/*基于muduo网络库开发的服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5.设置服务器端的线程数，muduo会自己划分I/O线程和worker线程
*/

class ChatServer
{
public:
    ChatServer(EventLoop *loop,               //事件循环，epoll，反应堆
               const InetAddress &listenAddr, // IP+port
               const string &nameArg)         //服务器的名字
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        //给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); // onConnection是非静态成员函数，有This指针，作为回调函数时会发生参数不匹配，所以得用绑定器

        //给服务器注册用户读写事件回调(已连接用户)
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        //设置服务器端的线程数量，1个I/O线程，3个worker线程
        _server.setThreadNum(4);
    }

    //开启事件循环
    void start()
    {
        _server.start();
    }

private:
    //专门处理用户的连接和断开 epoll listenfd  accept,
    void onConnection(const TcpConnectionPtr &conn)
    {
        // connected返回true就是有连接成功的
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() <<"state: online"<< endl;//toIpPort()是IP和端口号都打印出来，peerAdd是对端的，返回InetAddress类型
        } else {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() <<"state: offline"<< endl;
            conn->shutdown();//close(fd)
            //_loop->quit();
        }
    }

    //专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn, //通过这个连接可以发送数据，智能指针Tcpconnectionptr
                   Buffer *buffer,                  //缓冲区， 提高数据收发性能
                   Timestamp time)               //接收到数据的时间信息
    {
        string buf = buffer->retrieveAllAsString();//re..函数可以把缓冲区所有的内容都返回成字符串
        cout<<"recv data: "<< buf <<" time: "<< time.toString() << endl;//时间戳里的toString()可以将时间信息转成字符串
        conn->send(buf);//回声，收到啥发送啥
    }

    TcpServer _server; //第1步
    EventLoop *_loop;  //第2步，可以看成epoll，注册感兴趣的时间，如果发生，loop会给我们上报
};

int main(){
    EventLoop loop;//就像创建了epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "好菜");
    server.start(); //将listenfd 通过epoll_ctl添加到epoll上
    loop.loop();//就像epoll_wait(),以阻塞的方式等待用户的连接，和对已连接用户的读写事件等的操作
    return 0;
}