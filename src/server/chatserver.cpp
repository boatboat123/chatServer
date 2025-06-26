#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"


#include <functional>
#include <string>

using json=nlohmann::json;
using namespace std;
using namespace placeholders;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
    : server_(loop, listenAddr, nameArg), loop_(loop)
{
    // 注册连接和读写事件回调
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    //设置线程数量
    server_.setThreadNum(4);

}

void ChatServer::start()
{
    server_.start();
}

void ChatServer::onConnection(const TcpConnectionPtr&conn)
{
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
    
}

void ChatServer::onMessage(const TcpConnectionPtr& conn,Buffer* buffer,Timestamp time)
{
    string buf=buffer->retrieveAllAsString();
    json js=json::parse(buf);
    //通过js[msgid]绑定对应操作
    //完全解耦网络模块和业务模块的代码
    auto msgHandler=ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn,js,time);
}

