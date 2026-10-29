#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <iostream>

enum class UserStatus
{
    ONLINE,
        OFFLINE,
        AWAY,
        DO_NOT_DISTURB
        };

class User
{
  private:
    std::string userID;
    std::string username;
    std::string displayName;
    std::string email;
    std::string passwordHash;

    UserStatus status;
    bool banned;

  public:
    User(const std::string & userID,
         const std::string& username,
         const std::string& email,
         const std::string& passwordHash);

    //Getters
    std::string getUserID() const;
    std::string getUsername() const;
    std::string getDisplayName() const;
    std::string getEmail() const;
    UserStatus getStatus() const;
    bool isBanned() const;

    //Password Methods
    void changePassword(const std::string& newPasswordHash);
    bool verifyPassword(const std::string& password) const;

    //Setters
    void setUsername(const std::string & username);
    void setDisplayName(const std::string& displayName);
    



    //Status Methods
    void setStatus(UserStatus status);
    void setOnline();
    void setOffline();
    void setAway();
    void setDoNotDisturb();

    //Moderation
    void ban();
    void unban();

    //Debug
    void debugPrint() const;
};

#endif
