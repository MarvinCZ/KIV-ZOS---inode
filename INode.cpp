

#include "INode.hpp"

void INode::truncate(const std::shared_ptr<FileSystem>& fileSystem, int32_t newSize) {
    return std::make_shared<MemoryIterator>(this->shared_from_this(), fileSystem, true)->truncate(newSize);
}
