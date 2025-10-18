#include <string>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
std::string sha1(const std::string &data)
{
    std::string text = "hello world";
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(text.c_str()), text.size(), hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    
    return oss.str();
}