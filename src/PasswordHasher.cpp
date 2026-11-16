#include "PasswordHasher.h"

#include <argon2.h>
#include <sys/random.h>
#include <stdexcept>

std::string PasswordHasher::hashPassword(const std::string& password)
{
    // Generate a random 16-byte salt
    unsigned char salt[16];

    ssize_t bytesRead = getrandom(salt, sizeof(salt), 0);

    if (bytesRead != sizeof(salt))
    {
        throw std::runtime_error("Failed to generate random salt");
    }

    // Buffer for the encoded Argon2 hash
    char hash[256];

    int result = argon2id_hash_encoded(
        3,                  // iterations
        65536,              // memory in KiB
        1,                  // parallelism
        password.c_str(),   // password
        password.length(),
        salt,               // random salt
        sizeof(salt),       // salt length
        32,                 // hash length
        hash,
        sizeof(hash)
    );

    if (result != ARGON2_OK)
    {
        throw std::runtime_error(
            "Failed to hash password: " +
            std::string(argon2_error_message(result))
        );
    }

    return std::string(hash);
}

bool PasswordHasher::verifyPassword(
    const std::string& password,
    const std::string& storedHash)
{
    int result = argon2id_verify(storedHash.c_str(),
        password.c_str(), password.length()
    );

    return result == ARGON2_OK;
}
