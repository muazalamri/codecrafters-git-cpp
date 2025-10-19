#ifndef SHA_HPP
#define SHA_HPP
#include <string>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
std::string sha1(const std::string &text)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(text.c_str()), text.size(), hash);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        oss << std::setw(2) << static_cast<unsigned int>(hash[i]);

    return oss.str();
}
#endif // SHA_HPP