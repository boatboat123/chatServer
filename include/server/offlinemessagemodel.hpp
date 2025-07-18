#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include <string>
#include <vector>
using namespace std;
// 提供离线操作表的操作接口
class OfflineMsgModel
{
public:
    // 存储用户的离线消息
    void insert(int userid, string msg);
    // 删除用户的离线消息
    void remove(int userid);
    // 查询用户离线消息
    vector<string> query(int userid);
};

#endif