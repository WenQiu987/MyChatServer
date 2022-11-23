#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis(): _publish_context(nullptr), _subscribe_context(nullptr)
{

}

Redis::~Redis()
{
    if(_publish_context != nullptr){
        redisFree(_publish_context);
    }

    if (_subscribe_context != nullptr){
        redisFree(_subscribe_context);
    }
}

// 连接redis的函数
bool Redis::connect()
{
    // 负责发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379); // redis服务器的ip和端口号
    if(_publish_context == nullptr){
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责订阅消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if(_subscribe_context == nullptr){
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 写一个lambda表达式传给线程对象
    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报 ----不开辟单独线程的话会有问题：比如订阅后主线程一直阻塞在那等消息
    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;
}

// 在独立的线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply* reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribe_context, (void**)&reply))
    {
        // 订阅收到的消息是一个含有三个元素的数组
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息—— ——即调用服务器的函数，该函数会往id为通道号的用户发送消息————是服务器给用户发消息，不是redis
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str); // 参数0就是字符串"message",1是通道号，2是消息
        }
        freeReplyObject(reply);
    }
    // 否则就报错
    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}

// 初始化 向业务层上报通道消息 的回调函数
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}


// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply* reply = (redisReply*)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str()); // 返回值是动态生成的结构体，分配在堆内存所以需要释放
    if(nullptr == reply){
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply); // 释放内存
    return true;
}

// 向指定的通道订阅消息
bool Redis::subscribe(int channel)
{
    // 订阅消息命令SUBSCRIBE会造成线程阻塞，直到对应通道有消息了，所以这里把接收消息和订阅分开
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }

    // redisBufferWrite可以循环发送缓存区，直到缓存区数据发送完毕（done被置为1）
    int done = 0;
    while(!done){
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }

    return true;
}

// 向指定的通道 取消 订阅消息
bool Redis::unsubscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // 跟订阅 一模一样
    int done = 0;
    while(!done){
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}


