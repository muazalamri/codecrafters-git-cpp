#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include "read.hpp"
#include "write.hpp"
#include "sha.hpp"
#include "listdir.hpp"
int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!\n";

    // Uncomment this block to pass the first stage
    //
    if (argc < 2)
    {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }

    std::string command = argv[1];

    if (command == "init")
    {
        try
        {
            std::filesystem::create_directory(".git");
            std::filesystem::create_directory(".git/objects");
            std::filesystem::create_directory(".git/refs");

            std::ofstream headFile(".git/HEAD");
            if (headFile.is_open())
            {
                headFile << "ref: refs/heads/main\n";
                headFile.close();
            }
            else
            {
                std::cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }

            std::cout << "Initialized git directory\n";
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    else if (command == "ls-tree")
    {
        if (argc < 3)
        {
            std::cerr << "Usage : " << argv[0] << " ls-tree <obj>\n";
            return EXIT_FAILURE;
        }
        if (fs::is_directory(argv[argc - 1]))
        {
            fs::directory_iterator pathes = listdir(argv[argc - 1]);
            for (const auto &file_path : pathes)
            {
                //check if --name-only is provided
                if (argc >=4 && std::string(argv[2]) == "--name-only")
                {
                    std::cout << file_path.path().filename().string() << std::endl;
                }
                else{
                    std::cout << file_path.path().string() << std::endl;
                }
            }
        }
        //handle single file
        else if (fs::exists(argv[argc - 1]))
        {
            std::cout << argv[argc - 1] << std::endl;
        }
        else
        {
            std::cerr << "Path does not exist: " << argv[1] << std::endl;
            return EXIT_FAILURE;
        }
    }

    else if (command == "cat-file")
    {
        // read arg -p <hash>
        if (argc < 4 || std::string(argv[2]) != "-p")
        {
            std::cerr << "Usage : " << argv[0] << " cat-file -p <hash>\n";
            return EXIT_FAILURE;
        }
        std::string hash = argv[3];
        try
        {
            auto [header, content] = readZIP(hash);
            std::cout << content; // no newline added
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    else if (command == "hash-object")
    {
        // read arg -w <file>
        if (argc < 4 || std::string(argv[2]) != "-w")
        {
            std::cerr << "Usage : " << argv[0] << " hash-object -w <file>\n";
            return EXIT_FAILURE;
        }
        std::string filePath = argv[3];
        try
        {
            // Read file content
            std::ifstream file(filePath, std::ios::binary);
            if (!file)
            {
                std::cerr << "Cannot open file: " << filePath << '\n';
                return EXIT_FAILURE;
            }
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            // Prepare data with header
            std::string header = "blob " + std::to_string(content.size()) + '\0';
            std::string data = header + content;

            // Compute SHA-1 hash
            std::string hash = sha1(data);

            // Write compressed object to .git/objects
            writeZIP(hash, data);

            // Output the hash
            std::cout << hash << '\n';
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    else
    {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
