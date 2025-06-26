#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>

using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前系统登录的用户信息
User currentUser_;
// 记录当前登录用户的好友列表信息
vector<User> currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> currentUserGroupList;
// 显示当前登陆成功用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);

// 获取系统时间
string getCurrentTime();

// 主界面
void mainMenu();

// 聊天客户端程序实现 main线程用作发送线程 子线程用做接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }
    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    for (;;)
    {
        // 显示首页面菜单 登录注册退出
        cout << "=================" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.quit" << endl;
        cout << "=================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1: // login
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "user id:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            cout << "userpassword";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv login reponse error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else // 登陆成功
                    {
                        // 记录当前用户ID name
                        currentUser_.setId(responsejs["id"].get<int>());
                        currentUser_.setName(responsejs["name"]);
                        // 记录当前用户好友列表
                        if (responsejs.contains("friends"))
                        {
                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                currentUserFriendList.push_back(user);
                            }
                        }
                        // 记录当前用户的群组列表信息
                        if (responsejs.contains("groups"))
                        {
                            vector<string> vec1 = responsejs["groups"];
                            for (string &groupstr : vec1)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                vector<string> vec2 = grpjs["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }
                                currentUserGroupList.push_back(group);
                            }
                        }
                        // 显示用户登录的基本信息
                        showCurrentUserData();

                        // 显示当前用户的离线消息 个人聊天信息或者群组消息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                cout << js["time"] << "[" << js["id"] << "]" << js["name"]
                                     << "said:" << js["msg"] << endl;
                            }
                        }
                        // 登陆成功 启动线程接收数据
                        thread readTask(readTaskHandler, clientfd);
                        readTask.detach();
                        // 进入聊天主菜单
                        mainMenu();
                    }
                }
            }
            //break;
        }
        break;
        case 2: // register
        {
            char name[40] = {0};
            char pwd[40] = {0};
            cout << "username:";
            cin.getline(name, 40);
            cout << "password";
            cin.getline(pwd, 40);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv reg reponse error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) // 注册失败
                    {
                        cerr << name << "is already exist,regester error!" << endl;
                    }
                    else
                    {
                        cout << name << "register success,userid is" << responsejs["id"]
                             << ",do not forget it" << endl;
                    }
                }
            }
            //break;
        }
        break;
        case 3: // quit
        {
            close(clientfd);
            exit(0);
        }
        default:
            cerr << "invalid input" << endl;
            break;
        }
    }
    return 0;
}

void showCurrentUserData()
{
    cout << "======================login use========================" << endl;
    cout << "current login user=> id:" << currentUser_.getId() << "name:" << currentUser_.getName() << endl;
    cout << "----------------------friend list----------------------" << endl;
    if (!currentUserFriendList.empty())
    {
        for (User &user : currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "-----------------------group list-----------------------" << endl;
    if (!currentUserGroupList.empty())
    {
        for (Group &group : currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << user.getRole() << endl;
            }
        }
    }
    cout << "=========================================================" << endl;
}

// 获取系统时间
string getCurrentTime()
{
    // auto tt=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    // struct tm &ptm=*localtime(&tt);
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 转换为time_t类型
    std::time_t tt = std::chrono::system_clock::to_time_t(now);

    // 转换为本地时间结构
    std::tm tm = *std::localtime(&tt); // 注意：localtime返回指针需要解引用

    // 使用stringstream格式化输出
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

// 接收线程
void readTaskHandler(int clientfd) 
{
    while (true) {
        char buffer[4096] = {0};
        int len = recv(clientfd, buffer, sizeof(buffer), 0);
        if (len == -1 || len == 0) {
            // 连接错误或服务器关闭
            cerr << "Server connection lost!" << endl;
            close(clientfd);
            exit(-1);
        }

        // 解析收到的JSON数据
        json js = json::parse(buffer);
        
        // 处理不同类型的消息
        int msgType = js["msgid"].get<int>();
        switch (msgType) {
            case ONE_CHAT_MSG: // 私聊消息
                cout << "\n[" << js["time"] << "] " << js["fromname"] << "(" 
                     << js["id"] << ") said: " << js["msg"] << endl;
                break;
                
            case GROUP_CHAT_MSG: // 群聊消息
                cout << "\n[" << js["time"] << "] Group(" << js["groupid"] 
                     << ") " << js["fromname"] << " said: " << js["msg"] << endl;
                break;
                
            case LOGIN_MSG_ACK: // 登录响应(已在主线程处理)
                break;
                
            case REG_MSG_ACK: // 注册响应(已在主线程处理)
                break;
                
            default:
                cerr << "Invalid message type received!" << endl;
                break;
        }
        
        // 显示主菜单提示(保持界面友好)
        mainMenu(); 
    }
}


void mainMenu() {
    cout << "==================================" << endl;
    cout << "1. Private Chat | 2. Group Chat" << endl;
    cout << "3. Add Friend   | 4. Create Group" << endl;
    cout << "5. View Friends | 6. View Groups" << endl;
    cout << "7. Logout" << endl;
    cout << "==================================" << endl;
    cout << "Enter choice: ";
    cout.flush(); // 确保及时输出
}