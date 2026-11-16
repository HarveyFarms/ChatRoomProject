#include "User.h"
#include "PasswordHasher.h"

//Constructor
User::User(const std::string& userID_,
           const std::string& username_,
           const std::string& email_,
           const std::string& passwordHash_)
    : userID(userID_),
      username(username_),
      displayName(username_),
      email(email_),
      passwordHash(passwordHash_),
      status(UserStatus::OFFLINE),
      banned(false)
{}

//Getters

std::string User::getUserID() const
{
    return userID;
}

std::string User::getUsername() const
{
    return username;
}

std::string User::getDisplayName() const
{
    return displayName;
}

std::string User::getEmail() const
{
    return email;
}

UserStatus User::getStatus() const
{
    return status;
}

bool User::isBanned() const
{
    return banned;
}

//Setters

void User::setUsername(const std::string& username)
{
    this->username = username;
}

void User::setDisplayName(const std::string& displayName)
{
    this->displayName = displayName;
}

//Password methods

void User::changePassword(const std::string& newPasswordHash)
{
    this->passwordHash = newPasswordHash;
}
bool User::verifyPassword(const std::string& password) const
{
    return PasswordHasher::verifyPassword(password, passwordHash);
}

//Status methods

void User::setStatus(UserStatus status)
{
    this->status = status;
}

void User::setOnline()
{
    status = UserStatus::ONLINE;
}

void User::setOffline()
{
    status = UserStatus::OFFLINE;
}

void User::setAway()
{
    status = UserStatus::AWAY;
}

void User::setDoNotDisturb()
{
    status = UserStatus::DO_NOT_DISTURB;
}

//Moderation

void User::ban()
{
    banned = true;
}

void User::unban()
{
    banned = false;
}

void User::debugPrint() const
{
    std::cout << "----- User Debug Info -----\n"; 
    std::cout << "User ID: " << userID << '\n';
    std::cout << "Username: " << username << '\n';
    std::cout << "Display Name: " << displayName << '\n';
    std::cout << "Email: " << email << '\n';
    std::cout << "Status: ";

    switch (status) {
        case UserStatus::ONLINE:
            std::cout << "Online";
            break;
        case UserStatus::OFFLINE:
            std::cout << "Offline";
            break;
        case UserStatus::AWAY:
            std::cout << "Away";
            break;
        case UserStatus::DO_NOT_DISTURB:
            std::cout << "Do Not Disturb";
            break;
    }

    std::cout << "\nBanned: " << (banned ? "Yes" : "No");
    std::cout << "\n---------------------------\n"; 
}
