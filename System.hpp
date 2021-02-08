#pragma once
#include <memory>
#include "FileSystem.hpp"
#include "Directory.hpp"

class System {
public:
    explicit System(const std::string& file);
    int checkLoaded();
    int createDirectory(const std::string& path);
    int removeDirectory(const std::string& path);
    int listDirectory(const std::string& path);
    int printPwd();
    int cd(const std::string& path);
    int info(const std::string& path);
    int copyFromOutside(const std::string& sourcePath, const std::string& path);
    int copyToOutside(const std::string& outputPath, const std::string& path);
    int printFile(const std::string& path);
    int copyFile(const std::string& from, const std::string& to);
    int moveFile(const std::string& from, const std::string& to);
    int removeFile(const std::string& from);
    int hardLink(const std::string& from, const std::string& to);
    int format(unsigned long size);
    std::string pwd;

protected:
    std::shared_ptr<FileSystem> fileSystem;
    bool loaded;

    std::string getRealPath(const std::string &path);
    std::shared_ptr<Directory> getDirectory(const std::string& path, bool ignoreLast = false);
};



