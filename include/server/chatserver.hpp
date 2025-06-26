#include <mymuduo/TcpServer.h>
#include <iostream>

class ChatServer
{
private:
    TcpServer server_;
    EventLoop *loop_;

    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,Buffer*,Timestamp);
public:
    ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg);

    void start();
};