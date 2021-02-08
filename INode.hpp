#pragma once

#include <iterator>
#include <memory>
#include "structs.hpp"
#include "MemoryIterator.hpp"
#include "FileSystem.hpp"

class INode : public std::enable_shared_from_this<INode> {
    class OutputStream : public std::streambuf {
    public:
        explicit OutputStream(std::shared_ptr<MemoryIterator> memoryIterator) {
            this->memoryIterator = std::move(memoryIterator);
        }

        void close() {
            this->pubsync();
            this->memoryIterator->close();
        }
    protected:
        int_type overflow (int_type c) override {
            this->memoryIterator->writec(c);
            return c;
        }

        std::shared_ptr<MemoryIterator> memoryIterator;
    };
    class InputStream {
    public:
        explicit InputStream(std::shared_ptr<MemoryIterator> memoryIterator) {
            this->memoryIterator = std::move(memoryIterator);
        }

        int read(void* buffer, size_t size) {
            for (int i = 0; i < size; ++i) {
                int c = this->memoryIterator->readc();
                if (this->memoryIterator->readDone) {
                    return EOF;
                }
                *((char*)buffer + i) = (char) c;
            }

            return 0;
        }

    protected:
        std::shared_ptr<MemoryIterator> memoryIterator;
    };

public:
    explicit INode(std::shared_ptr<pseudo_inode> pseudoInode) {
        this->inode = std::move(pseudoInode);
    };

    OutputStream getOutputStream(const std::shared_ptr<FileSystem>& fileSystem) {
        return OutputStream(std::make_shared<MemoryIterator>(this->shared_from_this(), fileSystem, true));
    };
    InputStream getInputStream(const std::shared_ptr<FileSystem>& fileSystem) {
        return InputStream(std::make_shared<MemoryIterator>(this->shared_from_this(), fileSystem, false));
    };

    void truncate(const std::shared_ptr<FileSystem>& fileSystem, int32_t newSize = 0);
    std::shared_ptr<pseudo_inode> inode;

};



