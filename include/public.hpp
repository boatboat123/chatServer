#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/

enum EnMsgType
{
    LOGIN_MSG=1,
    LOGIN_MSG_ACK,
    REG_MSG,
    REG_MSG_ACK,  //注册响应消息
    ONE_CHAT_MSG, //聊天消息
    ADD_FRIEND_MSG //添加好友消息
};

#endif


//注册
//{"msgid":3,"name":2,"password":"123456"}
//登录
//{"msgid":1,"id":1,"password":"123"}
//{"msgid":1,"id":2,"password":"123456"}
//发消息
//{"msgid":5,"id":1,"from":"test","to":2,"msg":"hello boat"}
//{"msgid":5,"id":2,"from":"boat","to":1,"msg":"hello test"}