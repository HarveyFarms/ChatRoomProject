#include "Authentication.h"
#include "PasswordHasher.h"

#include <argon2.h>
#include <iostream>
#include <stdexcept>

Authentication::Authentication()
{}

Authentication::~Authentication()
{
    for (User* user : users)
    {
        delete user;
    }

    users.clear();
}

User* Authentication::registerUser(
    const std::string& username,
    const std::string& email,
    const std::string& password)
{
    // Check if username already exists
    if (findUserByUsername(username) != nullptr) {
        std::cout << "Username already exists.\n";
        return nullptr;
    }

    // Check if email already exists
    if (findUserByEmail(email) != nullptr) {
        std::cout << "Email already registered.\n";
        return nullptr;
    }

    // Hash the password
    std::string passwordHash =
        PasswordHasher::hashPassword(password);

    // Generate a simple ID
    std::string userId =
        std::to_string(users.size() + 1);

    // Create the user using the hash
    User* newUser = new User(
        userId,
        username,
        email,
        passwordHash
    );

    users.push_back(newUser);

    std::cout << "Account created successfully.\n";

    return newUser;
}

User* Authentication::login(
    const std::string& username,
    const std::string& password)
{
    // Find the user
    User* user = findUserByUsername(username);

    if (user == nullptr) {
        std::cout << "Invalid username or password.\n";
        return nullptr;
    }

    // Check if banned
    if (user->isBanned()) {
        std::cout << "This account has been banned.\n";
        return nullptr;
    }

    // Verify the password
    if (!user->verifyPassword(password)) {
        std::cout << "Invalid username or password.\n";
        return nullptr;
    }

    // Successful login
    user->setOnline();

    std::cout << "Login successful. Welcome, "
              << user->getUsername()
              << "!\n";

    return user;
}

void Authentication::logout(User* user)
{
    if (user == nullptr)
    {
        return;
    }

    user->setOffline();

    std::cout << user->getUsername() << " has logged out.\n";
}

User* Authentication::findUserByUsername(
    const std::string& username) const
{
    for (User* user : users)
    {
        if (user->getUsername() == username)
        {
            return user;
        }
    }
    // User not found
    return nullptr;
}

User* Authentication::findUserByEmail(
    const std::string& email) const
{
    for (User* user : users)
    {
        if (user->getEmail() == email)
        {
            return user;
        }
    }
    // User not found
    return nullptr;
}
