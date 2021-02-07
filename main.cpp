#include <memory>
#include "FileSystem.hpp"

struct pseudo_inode;


int main() {
    std::shared_ptr<FileSystem> fs = std::make_shared<FileSystem>("fs.dat");
//    fs->format(1024 * 1024 * 5);
    fs->load();

//    auto fileInode = fs->createInode();
//    auto output = fileInode->getOutputStream(fs);
//    FILE * file = fopen("../FileSystem.cpp", "rb");
//    int r;
//    while((r = fgetc(file)) != EOF) {
//        char c = (char) r;
//        output.sputn(&c, 1);
//    }
//    output.close();
//    std::cout << "FILE - " << fileInode->inode->file_size << "/" << MAX_FILE_SIZE << std::endl;

    auto inode = fs->getInode(1);
//    auto output = inode->getOutputStream(fs);
//    output.close();
    std::cout << inode->inode->file_size << " - " << (int)inode->inode->references << " - " << inode->inode->indirect1 << std::endl;
//    auto input = inode->getInputStream(fs);
//    char item;
//    while (input.read(&item, sizeof(char)) != EOF) {
//        std::cout << item;
//    }
    return 0;
}
