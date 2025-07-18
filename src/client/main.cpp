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
#include <unordered_map>

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
void mainMenu(int clientfd);

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
                        mainMenu(clientfd);
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
    auto tt=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm=localtime(&tt);
    char date[60]={0};
    sprintf(date,"%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year+1900,(int)ptm->tm_mon+1,(int)ptm->tm_mday,
            (int)ptm->tm_hour,(int)ptm->tm_min,(int)ptm->tm_sec);
    return string(date);
}

// 接收线程
void readTaskHandler(int clientfd) 
{
    while (true) {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, sizeof(buffer), 0);
        if (len == -1 || len == 0) {
            // 连接错误或服务器关闭
            cerr << "Server connection lost!" << endl;
            close(clientfd);
            exit(-1);
        }

        // 解析收到的JSON数据
        json js = json::parse(buffer);
        
        // 接收发来的消息
        if(ONE_CHAT_MSG==js["msgid"].get<int>())
        {
            cout<<js["time"].get<string>()<<"["<<js["id"]<<"]"<<js["name"].get<string>()
                <<"said:"<<js["msg"].get<string>()<<endl;
            continue;
        }
    }
}

void chat(int,string);
void help(int fd=0,string="");
void addfriend(int,string);
void creategroup(int,string);
void addgroup(int,string);
void groupchat(int,string);
void loginout(int,string);

unordered_map<string,string> commandMap={
    {"help","显示所有指令"},
    {"chat","one chat one,chat:friendid:message"},
    {"addfriend","addfriend:friendid"},
    {"creategroup","creategroup:groupname:groupdesc"}
};

unordered_map<string,function<void(int,string)>> commandHandlerMap={
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"loginout",loginout}
};

//主聊天页面程序
void mainMenu(int clientfd) {
    help();

    char buffer[1024]={0};
    for(;;)
    {
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command;//storage command
        int idx=commandbuf.find(":");
        if(-1==idx)
        {
            command=commandbuf;
        }
        else
        {
            command=commandbuf.substr(0,idx);
        }
        auto it=commandHandlerMap.find(command);
        if(it==commandHandlerMap.end())
        {
            cerr<<"invalid input command"<<endl;
            continue;
        }
        it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx));
    }
}

//"help" command handler
void help(int,string)
{
    cout<<"show command list>>>"<<endl;
    for(auto &p:commandMap)
    {
        cout<<p.first<<":"<<p.second<<endl;
    }
    cout<<endl;
}

//"addfriend" command handler
void addfriend(int clientfd,string str)
{
    int friendid=atoi(str.c_str());
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=currentUser_.getId();
    js["friendid"]=friendid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cout<<"send addfriend msg error->"<<buffer<<endl;
    }
}

//"chat" command handler
void chat(int clientfd,string str)
{
    int idx=str.find(":");
    if (-1==idx)
    {
        cerr<<"chat command invalid"<<endl;
        return;
    }
    
    int friendid=atoi(str.substr(0,idx).c_str());
    string message=str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=currentUser_.getId();
    js["name"]=currentUser_.getName();
    js["toid"]=friendid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
}

//void 