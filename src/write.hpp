#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <zlib.h>
void writeZIP(const std::string &hash, const std::string &data)
{
    //make sure directory .git/objects/xx exists
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