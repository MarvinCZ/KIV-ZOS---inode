#pragma once

#include <memory>
#include <vector>
#include "INode.hpp"
#include "FileSystem.hpp"

class Directory {
public:
    Directory(std::shared_ptr<INode> inode, std::shared_ptr<FileSystem> fileSystem);

    const std::vector<directory_item> &getItems() const;
    std::shared_ptr<INode> getItem(const std::string& name);
    std::shared_ptr<INode> getSelf();
    void addItem(const std::string& name, std::shared_ptr<INode> inode);
    void removeItem(const std::string& name, bool decrementSelfReference = false);
    std::shared_ptr<Directory> getParent();
    std::string getNameByInode(std::shared_ptr<INode> inode);

protected:
    std::shared_ptr<INode> inode;
    std::shared_ptr<FileSystem> fileSystem;
    std::vector<directory_item> items;

    void save();
};



