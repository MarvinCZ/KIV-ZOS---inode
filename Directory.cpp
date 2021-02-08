#include <cstring>
#include <utility>
#include "Directory.hpp"

Directory::Directory(std::shared_ptr<INode> inode, std::shared_ptr<FileSystem> fileSystem) {
    this->inode = std::move(inode);
    this->fileSystem = std::move(fileSystem);


    auto input = this->inode->getInputStream(this->fileSystem);
    directory_item item{};
    while (input.read(&item, sizeof(directory_item)) != EOF) {
        this->items.push_back(item);
    }
}

const std::vector<directory_item> &Directory::getItems() const {
    return items;
}

std::shared_ptr<INode> Directory::getItem(const std::string& name) {
    for (const directory_item &item : this->items) {
        if (name == item.item_name) {
            std::shared_ptr<INode> out = this->fileSystem->getInode(item.inode);

            return out;
        }
    }

    return nullptr;
}

std::shared_ptr<INode> Directory::getSelf() {
    return this->inode;
}

void Directory::addItem(const std::string& name, std::shared_ptr<INode> inode) {
    directory_item newItem{.inode = inode->inode->node_id};
    strcpy(newItem.item_name, name.substr(0, 11).c_str());

    this->items.push_back(newItem);
    if (inode->inode->isDirectory) {
        this->inode->inode->references++;
    }
    this->save();
}

void Directory::save() {
    auto output = this->inode->getOutputStream(this->fileSystem);
    for (directory_item item : this->items) {
        output.sputn(reinterpret_cast<const char *>(&item), sizeof(directory_item));
    }
    output.close();
    this->fileSystem->saveInode(this->inode->inode.get());
}

void Directory::removeItem(const std::string &name, bool decrementSelfReference) {
    auto it = this->items.begin();
    while (it != this->items.end())
    {
        if (it->item_name == name) {
            items.erase(it);
            break;
        }
        ++it;
    }
    if (decrementSelfReference) {
        this->inode->inode->references--;
    }
    this->save();
}

std::shared_ptr<Directory> Directory::getParent() {
    return std::make_shared<Directory>(this->getItem(".."), this->fileSystem);
}

std::string Directory::getNameByInode(std::shared_ptr<INode> inode) {
    for (directory_item item : this->items) {
        if (item.inode == inode->inode->node_id) {
            return item.item_name;
        }
    }
    return std::string();
}
