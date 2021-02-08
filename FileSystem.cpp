#include "FileSystem.hpp"
#include "structs.hpp"
#include "consts.hpp"

#include <utility>
#include <zconf.h>
#include <cstring>

FileSystem::FileSystem(std::string realFile) {
    this->realFile = std::move(realFile);
}

int FileSystem::format(unsigned long byteSize) {
    FILE * pFile = fopen(this->realFile.c_str(), "w");
    fclose(pFile);
    pFile = getFile();
    ftruncate(fileno(pFile), byteSize);

    superblock super_block{};
    super_block.disk_size = byteSize;

    long leftSize = byteSize - SUPERBLOCK_SIZE;
    long inodeSize = leftSize / (CLUSTER_SIZE_PER_INODE_SIZE + 1);
    long dataSize = leftSize - inodeSize;

    int inodeCount = (int) ((inodeSize * 8) / (INODE_SIZE * 8 + 1));
    int inodeMapSize = inodeCount / 8 + (inodeCount % 8 == 0 ? 0 : 1);

    if (inodeMapSize + INODE_SIZE * inodeCount > inodeSize) {
        inodeCount--;
        inodeMapSize = inodeCount / 8 + (inodeCount % 8 == 0 ? 0 : 1);
    }

    int clusterCount = (int) ((dataSize * 8) / (CLUSTER_SIZE * 8 + 1));
    int clusterMapSize = clusterCount / 8 + (clusterCount % 8 == 0 ? 0 : 1);

//    std::cout << inodeCount << ": " << inodeMapSize << " : " << clusterCount << ": " << clusterMapSize << std::endl;

    if (clusterMapSize + CLUSTER_SIZE * clusterCount > clusterCount) {
        clusterCount--;
        clusterMapSize = clusterCount / 8 + (clusterCount % 8 == 0 ? 0 : 1);
    }

    super_block.cluster_count = clusterCount;
    super_block.inode_count = inodeCount;
    super_block.bitmapi_start_address = SUPERBLOCK_SIZE + 1;
    super_block.bitmap_start_address = super_block.bitmapi_start_address + inodeMapSize;
    super_block.inode_start_address = super_block.bitmap_start_address + clusterMapSize;
    super_block.data_start_address = super_block.inode_start_address + inodeCount * INODE_SIZE;
    this->super_block = super_block;

    this->inodeBitmap.resize(super_block.inode_count, false);
    this->clusterBitmap.resize(super_block.cluster_count, false);

//    std::cout << super_block.inode_count << ": " << super_block.inode_start_address << " : " << super_block.cluster_count << ": " << super_block.data_start_address << std::endl;

    fseek(pFile, 0, SEEK_SET);
    fwrite(&super_block, SUPERBLOCK_SIZE, 1, pFile);

    char empty = '\0';
    for (uint32_t i = super_block.bitmapi_start_address; i < super_block.inode_start_address; ++i) {
        fseek(pFile, i, SEEK_SET);
        fwrite(&empty, 1, 1, pFile);
    }

    auto inode = createInode();
    inode->inode->isDirectory = true;
    inode->inode->references = 2;
    auto stream = inode->getOutputStream(this->shared_from_this());

    directory_item root{};
    root.inode = inode->inode->node_id;
    strcpy(root.item_name, ".");
    stream.sputn(reinterpret_cast<const char *>(&root), sizeof(directory_item));
    strcpy(root.item_name, "..");
    stream.sputn(reinterpret_cast<const char *>(&root), sizeof(directory_item));
    stream.close();

    releaseFile();
    return 0;
}

void FileSystem::load() {
    FILE * pFile = getFile();
    fseek(pFile, 0, SEEK_SET);
    fread(&this->super_block, SUPERBLOCK_SIZE, 1, pFile);

//    std::cout << this->super_block.inode_count << ": " << this->super_block.inode_start_address << " : " << this->super_block.cluster_count << ": " << this->super_block.data_start_address << std::endl;

    this->inodeBitmap.resize(this->super_block.inode_count);
    this->clusterBitmap.resize(this->super_block.cluster_count);
    int mask = 0;
    char byte;
    int32_t mapAddress = this->super_block.bitmapi_start_address;

    for (int i = 0; i < this->super_block.inode_count; ++i) {
        if (mask == 0) {
            mask = 1<<7;
            fseek(pFile, mapAddress++, SEEK_SET);
            fread(&byte, 1, 1, pFile);
        }
        if (this->inodeBitmap[i]) {
//            std::cout << "INODE - " << i << " - ON" << std::endl;
        }
        this->inodeBitmap[i] = (byte & mask) > 0;
        mask >>= 1;
    }

    mask = 0;
    mapAddress = this->super_block.bitmap_start_address;
    for (int i = 0; i < this->super_block.cluster_count; ++i) {
        if (mask == 0) {
            mask = 1<<7;
            fseek(pFile, mapAddress++, SEEK_SET);
            fread(&byte, 1, 1, pFile);
        }
        this->clusterBitmap[i] = (byte & mask) > 0;
        if (this->clusterBitmap[i]) {
//            std::cout << "CLUSTER - " << i << " - ON" << std::endl;
        }
        mask >>= 1;
    }

    releaseFile();
}

std::shared_ptr<INode> FileSystem::createInode() {
    for (int i = 0; i < this->inodeBitmap.size(); ++i) {
        if (!this->inodeBitmap[i]) {
            this->inodeBitmap[i] = true;
            this->setBit(i, true, this->super_block.bitmapi_start_address);

            std::shared_ptr<pseudo_inode> inode = std::make_shared<pseudo_inode>();
            inode->node_id = i;
            inode->file_size = 0;
            this->saveInode(inode.get());

            return std::make_shared<INode>(inode);
        }
    }
    return nullptr;
}

std::shared_ptr<INode> FileSystem::getInode(int index) {
    if (this->inodeBitmap[index]) {
        int32_t address = this->super_block.inode_start_address + (index * sizeof(pseudo_inode));
        std::shared_ptr<pseudo_inode> inode = std::make_shared<pseudo_inode>();
        this->read((void *) inode.get(), sizeof(pseudo_inode), address);

        return std::make_shared<INode>(inode);
    }
    return nullptr;
}

int32_t FileSystem::createCluster() {
    for (int i = 0; i < this->clusterBitmap.size(); ++i) {
        if (!this->clusterBitmap[i]) {
            this->clusterBitmap[i] = true;
            this->setBit(i, true, this->super_block.bitmap_start_address);
//            std::cout << "CREATING CLUSTER - " << i << " - ADDRESS - " << (i*CLUSTER_SIZE) << " - REAL ADDRESS - " << (this->super_block.data_start_address + (i*CLUSTER_SIZE)) << std::endl;
            return this->super_block.data_start_address + (i*CLUSTER_SIZE);
        }
    }
    return -1;
}

void FileSystem::read(void* buffer, size_t size, int32_t address) {
    FILE * pFile = getFile();
    fseek(pFile, address, SEEK_SET);
    fread(buffer, size, 1, pFile);
    releaseFile();
}

void FileSystem::write(void *buffer, size_t size, int32_t address) {
    FILE * pFile = getFile();
    fseek(pFile, address, SEEK_SET);
//    std::cout << "WRITING - " << address << " - " << size << std::endl;
    fwrite(buffer, size, 1, pFile);
    releaseFile();
}

void FileSystem::saveInode(const pseudo_inode* inode) {
    int32_t address = this->super_block.inode_start_address + (inode->node_id * sizeof(pseudo_inode));
    this->write((void *) inode, sizeof(pseudo_inode), address);
}

FILE *FileSystem::getFile() {
    if (fileCounter <= 0) {
        openedFile = fopen(this->realFile.c_str(), "r+b");
    }
    fileCounter++;
    return openedFile;
}

void FileSystem::releaseFile() {
    fileCounter--;
    if (fileCounter < 0) {
        fileCounter = 0;
    }
    if (fileCounter == 0 && openedFile != nullptr) {
        fclose(openedFile);
        openedFile = nullptr;
    }
}

void FileSystem::setBit(int32_t bit, bool state, int32_t address) {
    address += bit / 8;
    char byteState;
    this->read(&byteState, 1, address);
    if (state) {
        byteState |= (char)(1 << (7 - (bit % 8)));
    } else {
        byteState &= ~((char)(1 << (7 - (bit % 8))));
    }
    this->write(&byteState, 1, address);
}

void FileSystem::removeClusterByAddress(int32_t address) {
    address -= this->super_block.data_start_address;
    address /= CLUSTER_SIZE;

    this->clusterBitmap[address] = false;
    this->setBit(address, false, this->super_block.bitmap_start_address);
//    std::cout << "REMOVING CLUSTER - " << address << std::endl;
}

void FileSystem::removeInode(std::shared_ptr<pseudo_inode> inode) {
    this->inodeBitmap[inode->node_id] = false;
    this->setBit(inode->node_id, false, this->super_block.bitmapi_start_address);
}
