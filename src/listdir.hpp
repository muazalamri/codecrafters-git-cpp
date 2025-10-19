#ifndef LISTDIR_HPP
#define LISTDIR_HPP
#include <filesystem>
#include <string>
namespace fs=std::filesystem;
fs::directory_iterator listdir(std::string path)
{
    //handel . for current directory
    if (path == ".")
        path = fs::current_path().string();
    //handel ~ for home directory
    else if (path[0] == '~')
        path = std::string(getenv("HOME")) + path.substr(1);
    //handel .. for parent directory
    else if (path.substr(0, 3) == "../")
        path = fs::current_path().parent_path().string() + path.substr(2);
    return fs::directory_iterator(path);

}
#endif // LISTDIR_HPP