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
std::string writeTree(const std::string &dir = ".", bool print_hash = false)
{
    try
    {
        std::vector<tree_branch> entries;

        for (const auto &entry : fs::directory_iterator(dir))
        {
            std::string name = entry.path().filename().string();
            if (name == ".git")
                continue;

            std::string mode = entry.is_directory() ? "40000" : "100644";

            if (mode == "100644")
            {
                // ---- File (blob)
                std::ifstream file(entry.path(), std::ios::binary);
                if (!file)
                {
                    std::cerr << "Cannot open file: " << entry.path() << '\n';
                    continue;
                }

                std::string content((std::istreambuf_iterator<char>(file)), {});
                file.close();

                std::string blob_data = "blob " + std::to_string(content.size()) + '\0' + content;
                std::string hash = sha1(blob_data);
                writeZIP(hash, blob_data); // write .git/objects/xx/yyyy...

                entries.push_back({mode, name, hash});
            }
            else if (mode == "40000")
            {
                // ---- Directory (tree)
                std::string hash = writeTree(entry.path().string());
                entries.push_back({mode, name, hash});
            }
        }

        // Sort entries by name
        std::sort(entries.begin(), entries.end(),
                  [](const tree_branch &a, const tree_branch &b)
                  {
                      return a.name < b.name;
                  });

        // Build tree content
        std::string tree_content;
        for (const auto &e : entries)
        {
            std::string sha1_bin = hexToBinary(e.sha1);
            tree_content += e.mode + " " + e.name + '\0' + sha1_bin;
        }

        // Add header
        std::string data = "tree " + std::to_string(tree_content.size()) + '\0' + tree_content;
        std::string hash = sha1(data);

        writeZIP(hash, data);
        if (print_hash)
            std::cout << hash << std::endl;

        return hash;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return "";
    }
}
