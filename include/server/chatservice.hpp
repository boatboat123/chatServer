#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "mymuduo/TcpConnection.h"
#include "mymuduo/Callbacks.h"
#include "json.hpp"
#include "UserModel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
using json=nlohmann::json;

#include <unordered_map>
#include <functional>
#include <mutex>
#include <string>

using namespace std;



using MsgHandler=function<void(const TcpConnectionPtr &conn,json &js,Timestamp)>;
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    void login(const TcpConnectionPtr &conn,json &js,Timestamp time);
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //服务器异常 业务重置
    void reset();
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js,Timestamp time);
    //添加好友
    void addFriend(const TcpConnectionPtr &conn, json &js,Timestamp time);
private:
    ChatService();
    //存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler> msgHandlerMap_;

    //存储用户聊天id和tcpconnection
    unordered_map<int,TcpConnectionPtr> connMap_;

    //定义互斥锁 
    mutex connMutex_;
    //数据操作类对象
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
};
#endif