#ifndef GROUPUSER_H
#define GROUPUSER_H
#include "user.hpp"

class GroupUser : public User
{
public:
    GroupUser(int id = -1, string name = "", string email = "")
        : User(id, name, email) {}

    void setRole(string role)
    {
        this->role = role;
    }
    string getRole() const
    {
        return this->role;
    }
private: 
    string role; // 角色，例如 "admin", "member"
};


#endif // GROUPUSER_H