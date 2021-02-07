#pragma once

#include <memory>
#include "consts.hpp"

class INode;
class FileSystem;

class MemoryIterator {
public:
    MemoryIterator(std::shared_ptr<INode> inode, std::shared_ptr<FileSystem> fileSystem, bool write);
    void rewind();
    void next();
    int32_t address();
    int32_t clusterAddress(int cluster);
    void truncate(int32_t truncateSize);
    void writec(char c);
    int readc();
    void close();
protected:
    std::shared_ptr<INode> inode;
    std::shared_ptr<FileSystem> fileSystem;
    bool write;
    int32_t used_clusters;
    int32_t index;
    int32_t new_size;
};



