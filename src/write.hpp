#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <string>
#include <zlib.h>
#include "sha.hpp"
#include "listdir.hpp"
std::string hexToBinary(const std::string &hex)
{
    std::string binary;
    binary.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        char byte = static_cast<char>(strtol(byteString.c_str(), nullptr, 16));
        binary.push_back(byte);
    }
    return binary;
}
void writeZIP(const std::string &hash, const std::string &data)
{
    // make sure directory .git/objects/xx exists
    std::string dir = ".git/objects/" + hash.substr(0, 2);
    std::filesystem::create_directories(dir);
    std::string path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    std::ofstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("لا يمكن فتح الملف للكتابة: " + path);
    // Compress data using zlib
    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    stream.avail_in = data.size();
    if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
        throw std::runtime_error("فشل تهيئة zlib للضغط");
    std::vector<unsigned char> buffer(4096);
    int ret;
    do
    {
        stream.next_out = buffer.data();
        stream.avail_out = buffer.size();
        ret = deflate(&stream, Z_FINISH);
        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            deflateEnd(&stream);
            throw std::runtime_error("خطأ أثناء الضغط");
        }
        size_t have = buffer.size() - stream.avail_out;
        file.write(reinterpret_cast<char *>(buffer.data()), have);
    } while (ret != Z_STREAM_END);
    deflateEnd(&stream);
    file.close();
}
void writeTree()
{
    try
    {
        std::vector<tree_branch> entries;
        fs::directory_iterator dirIt = listdir(".");
        for (const auto &entry : dirIt)
        {
            std::string name = entry.path().filename().string();
            if (name == ".git")
                continue; // Skip .git directory
            std::string mode = entry.is_directory() ? "40000" : "100644";
            entries.push_back({mode, name, sha1(entry.path().string())});
        }
        // Build tree object content
        std::string tree_content;
        for (const auto &entry : entries)
        {
            std::string sha1_bin = hexToBinary(entry.sha1);
            tree_content += entry.mode + " " + entry.name + '\0' + sha1_bin;
        }

        // Prepare data with header
        std::string header = "tree " + std::to_string(tree_content.size()) + '\0';
        std::string data = header + tree_content;

        // Compute SHA-1 hash
        std::string hash = sha1(data);
        std::cout << hash << std::endl;
        // Write compressed object to .git/objects
        writeZIP(hash, data);

        // Output the hash
        std::cout << hash << '\n';
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}