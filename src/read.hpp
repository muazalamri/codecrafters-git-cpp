#include <iostream>
#include <fstream>
#include <vector>
#include <zlib.h>
#include <sstream>
#include <iomanip>

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
    std::string mode;
    std::string name;
    std::string sha1;
};

struct commit
{
    std::string tree;
    std::string author;
    std::string author_email;
    std::string author_date;
    std::string committer;
    std::string committer_email;
    std::string committer_date;
    std::string message;
};
commit read_commit(const std::string &hash)
{
    auto compressed = readFile(".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2));
    auto decompressed = decompressZlib(compressed);
    int pos;
    pos=decompressed.find("tree")+1;
    std::string tree=decompressed.substr(pos,decompressed.find("\n",pos)-pos);
    pos=decompressed.find("author")+1;
    std::string author=decompressed.substr(pos,decompressed.find(" ",pos)-pos);
    pos=decompressed.find("<",pos)+1;
    std::string author_Email= decompressed.substr(pos,decompressed.find(">",pos)-pos);
    pos=decompressed.find("> ",pos);
    std::string author_date= decompressed.substr(pos,decompressed.find("\n",pos)-pos);
    pos=decompressed.find("committer")+1;
    std::string committer=decompressed.substr(pos,decompressed.find(" ",pos)-pos);
    pos=decompressed.find("<",pos)+1;
    std::string committer_Email= decompressed.substr(pos,decompressed.find(">",pos)-pos);
    pos=decompressed.find("> ",pos);
    std::string committer_date= decompressed.substr(pos,decompressed.find("\n",pos)-pos);
    pos=decompressed.find("\n\n")+2;
    std::string message= decompressed.substr(pos);
    return {tree,author,author_Email,author_date,committer,committer_Email,committer_date,message};
}
std::string toHex(const unsigned char *data, size_t len)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i)
        oss << std::setw(2) << static_cast<unsigned int>(data[i]);
    return oss.str();
}

std::vector<tree_branch> readTree(const std::string &hash,bool print_data=false,bool print_names=false)
{
    std::string path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    auto compressed = readFile(path);
    std::string decompressed = decompressZlib(compressed);

    // تخطي ترويسة "tree <size>\0"
    size_t header_end = decompressed.find('\0') + 1;
    size_t pos = header_end;

    std::vector<tree_branch> entries;

    while (pos < decompressed.size())
    {
        // 1️⃣ اقرأ الـ mode حتى أول space
        size_t space_pos = decompressed.find(' ', pos);
        std::string mode = decompressed.substr(pos, space_pos - pos);
        pos = space_pos + 1;

        // 2️⃣ اقرأ الاسم حتى أول '\0'
        size_t null_pos = decompressed.find('\0', pos);
        std::string name = decompressed.substr(pos, null_pos - pos);
        pos = null_pos + 1;

        // 3️⃣ اقرأ 20 بايت التالية كـ hash ثنائي
        std::string sha1_bin = decompressed.substr(pos, 20);
        pos += 20;

        std::string sha1_hex = toHex(reinterpret_cast<const unsigned char *>(sha1_bin.data()), 20);

        if(print_names){std::cout << name<<std::endl;}
        if(print_data){std::cout << mode << " " << name << " " << sha1_hex << std::endl;}
        entries.push_back({mode, name, sha1_hex});
    }

    return entries;
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