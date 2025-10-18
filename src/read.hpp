#include <iostream>
#include <fstream>
#include <vector>
#include <zlib.h>

std::vector<unsigned char> readFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("لا يمكن فتح الملف: " + path);
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
}

std::string decompressZlib(const std::vector<unsigned char> &data)
{
    z_stream stream{};
    stream.next_in = const_cast<Bytef *>(data.data());
    stream.avail_in = data.size();

    if (inflateInit(&stream) != Z_OK)
        throw std::runtime_error("فشل تهيئة zlib");

    std::string out;
    std::vector<unsigned char> buffer(4096);

    int ret;
    do
    {
        stream.next_out = buffer.data();
        stream.avail_out = buffer.size();

        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            inflateEnd(&stream);
            throw std::runtime_error("خطأ أثناء فك الضغط");
        }

        size_t have = buffer.size() - stream.avail_out;
        out.append(reinterpret_cast<char *>(buffer.data()), have);
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    return out;
}
struct tree_branch
{
    std::string ftype, fname, fhash;
    tree_branch(std::string file_type, std::string file_name, std::string file_hash) : ftype(file_type), fname(file_name), fhash(fhash) {}
    tree_branch(std::string line)
    {
        ftype = line.substr(0, line.find(' '));
        line = line.substr(line.find(' ') + 1);
        fname = line.substr(0, line.find(' '));
        fhash = line.substr(line.find(' ') + 1);
    }
};

std::vector<tree_branch> readTree(std::string hash)
{
    std::string path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    auto compressed = readFile(path);
    std::string decompressed = decompressZlib(compressed);
    std::string line;
    std::vector<tree_branch> files;
    while (decompressed.size()>0)
    {
        line = decompressed.substr(0, decompressed.find('\n'));
        std::cout << "line : " << line << std::endl;
        if(line.size()<47){ //5 type +space+name+space+40 hash
            break;
        }
        files.push_back(tree_branch(line));
    }
    return files;
}
std::pair<std::string, std::string> readZIP(std::string hash)
{
    try
    {
        std::string path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
        auto compressed = readFile(path);
        std::string decompressed = decompressZlib(compressed);
        // split header and content
        size_t null_pos = decompressed.find('\0');
        std::string header = decompressed.substr(0, null_pos);
        std::string content = decompressed.substr(null_pos + 1);
        return {header, content};
    }
    catch (const std::exception &e)
    {
        std::cerr << "error : " << e.what() << std::endl;
    }
}