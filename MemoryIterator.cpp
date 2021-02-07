#include "MemoryIterator.hpp"
#include "FileSystem.hpp"
#include "INode.hpp"

MemoryIterator::MemoryIterator(std::shared_ptr <INode> inode, std::shared_ptr<FileSystem> fileSystem, bool write = false) {
    this->inode = std::move(inode);
    this->fileSystem = std::move(fileSystem);
    this->write = write;
    if (write) {
        this->new_size = 0;
    }

    this->used_clusters = (this->inode->inode->file_size - 1) / CLUSTER_SIZE;

    this->rewind();
}


void MemoryIterator::writec(char c) {
    int32_t address = this->address();
    this->fileSystem->write(&c, sizeof(char), address);
    this->next();
}

int MemoryIterator::readc() {
    if (this->index >= this->inode->inode->file_size) {
        return EOF;
    }
    int32_t address = this->address();
    char c;
    this->fileSystem->read(&c, sizeof(char), address);
    this->next();
    return c;
}

void MemoryIterator::close() {
    if (this->write) {
        if (this->new_size < this->inode->inode->file_size) {
            this->truncate(new_size);
        }
        this->inode->inode->file_size = new_size;
        this->fileSystem->saveInode(this->inode->inode.get());
        // TODO truncate overflow
    }
}

void MemoryIterator::rewind() {
    this->index = 0;
}

void MemoryIterator::next() {
    this->index++;
    // TODO check for max
}

int32_t MemoryIterator::address() {
    int cluster = this->index / CLUSTER_SIZE;
    int rest = this->index % CLUSTER_SIZE;
//    std::cout << "GETTING - CLUSTER - " << cluster << " - " << rest << std::endl;

    if (this->write && this->index > this->new_size) {
        this->new_size = this->index + 1;
    }
    int32_t address = this->clusterAddress(cluster) + rest;
//    std::cout << "GETTING ADDRESS - " << address << std::endl;

    return address;
}

int32_t MemoryIterator::clusterAddress(int cluster) {
    int32_t new_cluster = -1;
    if (this->write && this->used_clusters <= cluster) {
        new_cluster = this->fileSystem->createCluster();
        this->used_clusters++;
    }
    if (cluster >= 0 && cluster < 5) {
        if (new_cluster != -1) {
            // new cluster to direct link
            this->inode->inode->direct[cluster] = new_cluster;
        }
        return this->inode->inode->direct[cluster];
    }

    // INDIRECT 1
    cluster -= 5;
    if (cluster < LINKS_PER_CLUSTER) {
        if (new_cluster != -1 && cluster == 0) {
            // first indirect - cluster for links
            int new_indirect = this->fileSystem->createCluster();

            this->inode->inode->indirect1 = new_indirect;
//            std::cout << "CREATING INDIRECT1 - " << new_indirect << std::endl;
        }
        int32_t address = this->inode->inode->indirect1 + (cluster * sizeof(int32_t));
        if (new_cluster != -1) {
            std::cout << "INDIRECT1 - " << address << " - CLUSTER - " << cluster << "/" << LINKS_PER_CLUSTER << " - NEW - " << new_cluster << std::endl;
            // indirect - cluster address in indirect cluster
            this->fileSystem->write(&new_cluster, sizeof(int32_t), address);
            return new_cluster;
        }
        int32_t out;
        this->fileSystem->read(&out, sizeof(int32_t), address);
        return out;
    }

    // INDIRECT 2
    cluster -= LINKS_PER_CLUSTER;
    int root = cluster / LINKS_PER_CLUSTER;
    cluster %= LINKS_PER_CLUSTER;
    if (root >= LINKS_PER_CLUSTER) {
        // overflow
        return -1;
    }
    if (new_cluster != -1 && cluster == 0 && root == 0) {
        // first indirect2 - cluster for links of links
        int new_indirect = this->fileSystem->createCluster();

        this->inode->inode->indirect2 = new_indirect;
    }
    // READ INDIRECT 2 -> READ INDIRECT -> clusterAddress
    int32_t address = this->inode->inode->indirect2 + (root * sizeof(int32_t));
    if (new_cluster != -1 && cluster == 0) {
        // first link in new cluster of links
        int new_indirect = this->fileSystem->createCluster();

        this->fileSystem->write(&new_indirect, sizeof(int32_t), address);
    }
    this->fileSystem->read(&address, sizeof(int32_t), address);
    address += (cluster * sizeof(int32_t));
    if (new_cluster != -1) {
        this->fileSystem->write(&new_cluster, sizeof(int32_t), address);
        return new_cluster;
    }
    this->fileSystem->read(&address, sizeof(int32_t), address);

    return address;
}

void MemoryIterator::truncate(int32_t truncateSize) {
    int to = (truncateSize - 1) / CLUSTER_SIZE;
    int from = this->used_clusters;

    if (to < 4) {
        int max = from > 4 ? 4 : from;
        for (int i = to; i <= max; ++i) {
            this->fileSystem->removeClusterByAddress(this->inode->inode->direct[i]);
        }
    }
    if (from < 5) {
        return;
    }

    // INDIRECT 1
    from -= 5;
    to -= 5;
    to = to > 0 ? to : 0;
    if (to < LINKS_PER_CLUSTER) {
        int max = from > LINKS_PER_CLUSTER ? LINKS_PER_CLUSTER : from;
        for (int i = to; i < max; ++i) {
            int32_t address = this->inode->inode->indirect1 + (i * sizeof(int32_t));
            int32_t out;
            this->fileSystem->read(&out, sizeof(int32_t), address);
            this->fileSystem->removeClusterByAddress(out);
        }
    }

    if (to < 5) {
        this->fileSystem->removeClusterByAddress(this->inode->inode->indirect1);
    }
    if (from < LINKS_PER_CLUSTER) {
        return;
    }

    // INDIRECT 2
    from -= LINKS_PER_CLUSTER;
    to -= LINKS_PER_CLUSTER;
    to = to > 0 ? to : 0;

    // READ INDIRECT 2 -> READ INDIRECT -> clusterAddress
    int root = to / LINKS_PER_CLUSTER;
    int32_t rootAddress = this->inode->inode->indirect2 + (root * sizeof(int32_t));
    this->fileSystem->read(&rootAddress, sizeof(int32_t), rootAddress);
    int cluster = to % LINKS_PER_CLUSTER;
    bool fromStart = cluster == 0;
    for (int i = to; i <= from; ++i) {
        if (cluster >= LINKS_PER_CLUSTER) {
            if (fromStart) {
                this->fileSystem->removeClusterByAddress(rootAddress);
            }
            fromStart = true;
            root++;
            rootAddress = this->inode->inode->indirect2 + (root * sizeof(int32_t));
            this->fileSystem->read(&rootAddress, sizeof(int32_t), rootAddress);
            cluster %= LINKS_PER_CLUSTER;
        }

        int32_t address = rootAddress + (cluster * sizeof(int32_t));
        this->fileSystem->read(&address, sizeof(int32_t), address);
        this->fileSystem->removeClusterByAddress(address);
        cluster++;
    }

    if (fromStart) {
        this->fileSystem->removeClusterByAddress(rootAddress);
    }
    if (to <= 0) {
        this->fileSystem->removeClusterByAddress(this->inode->inode->indirect2);
    }
}
