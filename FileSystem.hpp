#pragma once

#include <string>
#include <memory>
#include <vector>
#include "INode.hpp"

class FileSystem : public std::enable_shared_from_this<FileSystem> {
public:
    explicit FileSystem(std::string realFile);

    int format(long byteSize);

    std::shared_ptr<INode> createInode();
    std::shared_ptr<INode> getInode(int index);
    int32_t createCluster();

    void load();
    void read(void* buffer, size_t size, int32_t address);
    void write(void* buffer, size_t size, int32_t address);
    void removeClusterByAddress(int32_t address);
    void saveInode(const pseudo_inode* inode);

private:
    std::string realFile;
    superblock super_block;
    std::vector<bool> inodeBitmap;
    std::vector<bool> clusterBitmap;
    FILE* openedFile = nullptr;
    FILE* getFile();
    void releaseFile();
    int fileCounter = 0;
    void setBit(int32_t bit, bool state, int32_t address);
};



