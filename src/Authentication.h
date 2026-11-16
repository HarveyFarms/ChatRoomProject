#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <string>
#include <vector>
#include "User.h"

class Authentication
{
private:
    std::vector<User*> users;
public:
    Authentication();
    ~Authentication();
    
    User* registerUser(const std::string& username, const std::string& email,
                       const std::string& password);

    User* login( const std::string& username, const std::string& password);
    void logout(User* user);

    User* findUserByUsername(const std::string& username) const;

    User* findUserByEmail(const std::string& email) const;
};

#endif
