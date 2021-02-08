#include <cstring>
#include <stack>
#include "unistd.h"
#include "System.hpp"

System::System(const std::string& file) {
    this->fileSystem = std::make_shared<FileSystem>(file);
    this->loaded = false;
    if (access(file.c_str(), R_OK) == 0) {
        this->fileSystem->load();
        this->loaded = true;
    }
    this->pwd = "/";
}

int System::checkLoaded() {
    if (this->loaded) {
        return 0;
    }

    std::cerr << "FILESYSTEM NOT LOADED" << std::endl;
    return 10;
}

std::shared_ptr<Directory> System::getDirectory(const std::string& path, bool ignoreLast) {
    auto directoryInode = this->fileSystem->getInode(0); // root directory
    auto directory = std::make_shared<Directory>(directoryInode, this->fileSystem);
    if (path == "/") {
        return directory;
    }
    auto start = 1;
    auto end = path.find('/', start);
    std::string part;
    while (end != std::string::npos && directory != nullptr)
    {
        part = path.substr(start, end - start);
        directoryInode = directory->getItem(part);
        if (directoryInode == nullptr || !directoryInode->inode->isDirectory) {
            return nullptr;
        }
        directory = std::make_shared<Directory>(directoryInode, this->fileSystem);
        start = end + 1;
        end = path.find('/', start);
    }

    if (ignoreLast) {
        return directory;
    }

    part = path.substr(start, end);
    if (!part.empty()) {
        directoryInode = directory->getItem(part);
        if (directoryInode == nullptr || !directoryInode->inode->isDirectory) {
            return nullptr;
        }

        return std::make_shared<Directory>(directoryInode, this->fileSystem);
    }

    return directory;
}

int System::createDirectory(const std::string &path) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string realPath = getRealPath(path);

    std::shared_ptr<Directory> parent = this->getDirectory(realPath, true);
    if (parent == nullptr) {
        std::cerr << "PATH NOT FOUND" << std::endl;
        return 1; // directory not found
    }
    std::string dirname = realPath.substr(realPath.find_last_of('/') + 1);
    std::shared_ptr<INode> item = parent->getItem(dirname);
    if (item != nullptr) {
        std::cerr << "EXISTS" << std::endl;
        return 2; // file exists
    }

    auto inode = this->fileSystem->createInode();
    inode->inode->isDirectory = true;
    inode->inode->references = 2;
    auto stream = inode->getOutputStream(this->fileSystem);

    directory_item directoryItem{};
    strcpy(directoryItem.item_name, ".");
    directoryItem.inode = inode->inode->node_id;
    stream.sputn(reinterpret_cast<const char *>(&directoryItem), sizeof(directory_item));
    strcpy(directoryItem.item_name, "..");
    directoryItem.inode = parent->getSelf()->inode->node_id;
    stream.sputn(reinterpret_cast<const char *>(&directoryItem), sizeof(directory_item));
    stream.close();

    parent->addItem(dirname, inode);

    std::cout << "OK" << std::endl;
    return 0;
}

int System::removeDirectory(const std::string &path) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string realPath = getRealPath(path);

    if (realPath == "/") {
        return 3;
    }

    std::shared_ptr<Directory> parent = this->getDirectory(realPath, true);
    std::shared_ptr<Directory> directory = this->getDirectory(realPath, false);

    if (directory == nullptr) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 1; // not found
    }

    if (directory->getItems().size() > 2) {
        std::cerr << "NOT EMPTY" << std::endl;
        return 2; // not empty
    }

    std::string dirname = realPath.substr(realPath.find_last_of('/') + 1);

    if (dirname == "." || dirname == "..") {
        return 3;
    }

//    parent->removeItem(dirname, parent->getSelf()->inode->node_id == directory->getItem("..")->inode->node_id);
    // no hardlink on directory decrementing always
    parent->removeItem(dirname, true);

    directory->getSelf()->truncate(this->fileSystem);
    this->fileSystem->removeInode(directory->getSelf()->inode);

    std::cout << "OK" << std::endl;
    return 0;
}

int System::listDirectory(const std::string &path) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string realPath = getRealPath(path);
    std::shared_ptr<Directory> directory = this->getDirectory(realPath, false);

    if (directory == nullptr) {
        std::cerr << "PATH NOT FOUND" << std::endl;
        return 1;
    }

    for (const directory_item& item : directory->getItems()) {
        std::shared_ptr<INode> inode = this->fileSystem->getInode(item.inode);
        std::cout << (inode->inode->isDirectory ? '-' : '+') << item.item_name << std::endl;
    }

    return 0;
}

std::string System::getRealPath(const std::string &path) {
    if (path[0] == '/') {
        return path;
    }

    if (this->pwd == "/") {
        return "/" + path;
    }

    return this->pwd + "/" + path;
}

int System::printPwd() {
    std::cout << this->pwd << std::endl;

    return 0;
}

int System::cd(const std::string &path) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string realPath = getRealPath(path);
    std::shared_ptr<Directory> directory = this->getDirectory(realPath, false);

    if (directory == nullptr) {
        std::cerr << "PATH NOT FOUND" << std::endl;
        return 1;
    }

    std::stack<std::string> pathStack;
    std::shared_ptr<Directory> parent;
    while (directory->getSelf()->inode->node_id != 0) {
        parent = directory->getParent();
        pathStack.push(parent->getNameByInode(directory->getSelf()));
        directory = parent;
    }
    this->pwd = std::string();
    while (!pathStack.empty()) {
        this->pwd += "/" + pathStack.top();
        pathStack.pop();
    }
    if (this->pwd.empty()) {
        this->pwd = "/";
    }

    std::cout << "OK" << std::endl;
    return 0;
}

int System::info(const std::string &path) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string realPath = getRealPath(path);
    std::shared_ptr<Directory> directory = this->getDirectory(realPath, true);

    if (directory == nullptr) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 1;
    }

    std::string filename = realPath.substr(realPath.find_last_of('/') + 1);
    std::shared_ptr<INode> file = realPath == "/" ? directory->getSelf() : directory->getItem(filename);

    if (file == nullptr) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 1;
    }

    std::cout << realPath << " - " << file->inode->file_size << " - i-node " << file->inode->node_id << std::endl;
    return 0;
}

int System::copyFromOutside(const std::string &sourcePath, const std::string &path) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string realPath = getRealPath(path);
    std::shared_ptr<Directory> directory = this->getDirectory(realPath, true);

    if (directory == nullptr) {
        std::cerr << "PATH NOT FOUND" << std::endl;
        return 1;
    }

    std::string filename = realPath.substr(realPath.find_last_of('/') + 1);
    if (directory->getItem(filename) != nullptr) {
        std::cerr << "FILE EXISTS" << std::endl;
        return 2;
    }

    if (access(sourcePath.c_str(), R_OK) != 0) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 3;
    }

    std::shared_ptr<INode> fileInode = this->fileSystem->createInode();
    fileInode->inode->references = 1;
    auto output = fileInode->getOutputStream(this->fileSystem);
    FILE * file = fopen(sourcePath.c_str(), "rb");
    int r;
    while((r = fgetc(file)) != EOF) {
        char c = (char) r;
        output.sputn(&c, 1);
    }
    output.close();
    fclose(file);

    directory->addItem(filename, fileInode);

    std::cout << "OK" << std::endl;
    return 0;
}

int System::copyToOutside(const std::string &outputPath, const std::string &path) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string realPath = getRealPath(path);
    std::shared_ptr<Directory> directory = this->getDirectory(realPath, true);

    if (directory == nullptr) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 1;
    }

    std::string filename = realPath.substr(realPath.find_last_of('/') + 1);
    std::shared_ptr<INode> file = directory->getItem(filename);
    if (file == nullptr || file->inode->isDirectory) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 2;
    }

    FILE * outFile = fopen(outputPath.c_str(), "wb");
    auto input = file->getInputStream(this->fileSystem);
    char item;
    while (input.read(&item, sizeof(char)) != EOF) {
        fputc(item, outFile);
    }
    fclose(outFile);

    std::cout << "OK" << std::endl;
    return 0;
}

int System::printFile(const std::string &path) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string realPath = getRealPath(path);
    std::shared_ptr<Directory> directory = this->getDirectory(realPath, true);

    if (directory == nullptr) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 1;
    }

    std::string filename = realPath.substr(realPath.find_last_of('/') + 1);
    std::shared_ptr<INode> file = directory->getItem(filename);
    if (file == nullptr || file->inode->isDirectory) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 2;
    }

    auto input = file->getInputStream(this->fileSystem);
    char item;
    while (input.read(&item, sizeof(char)) != EOF) {
        std::cout << item;
    }
    std::cout << std::endl;
    return 0;
}

int System::copyFile(const std::string &from, const std::string &to) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string fromPath = getRealPath(from);
    std::shared_ptr<Directory> fromDirectory = this->getDirectory(fromPath, true);

    std::string toPath = getRealPath(to);
    std::shared_ptr<Directory> toDirectory = this->getDirectory(toPath, true);

    if (fromDirectory == nullptr) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 1;
    }

    if (toDirectory == nullptr) {
        std::cerr << "PATH NOT FOUND" << std::endl;
        return 1;
    }

    std::string fromFilename = fromPath.substr(fromPath.find_last_of('/') + 1);
    std::string toFilename = toPath.substr(toPath.find_last_of('/') + 1);
    std::shared_ptr<INode> inputFile = fromDirectory->getItem(fromFilename);
    if (inputFile == nullptr || inputFile->inode->isDirectory) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 2;
    }

    if (toDirectory->getItem(toFilename) != nullptr) {
        std::cerr << "FILE EXISTS" << std::endl;
        return 3;
    }

    auto input = inputFile->getInputStream(this->fileSystem);
    std::shared_ptr<INode> fileInode = this->fileSystem->createInode();
    fileInode->inode->references = 1;
    auto output = fileInode->getOutputStream(this->fileSystem);
    char item;
    while (input.read(&item, sizeof(char)) != EOF) {
        output.sputn(&item, 1);
    }
    output.close();
    toDirectory->addItem(toFilename, fileInode);

    std::cout << "OK" << std::endl;

    return 0;
}

int System::moveFile(const std::string &from, const std::string &to) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string fromPath = getRealPath(from);
    std::shared_ptr<Directory> fromDirectory = this->getDirectory(fromPath, true);

    std::string toPath = getRealPath(to);
    std::shared_ptr<Directory> toDirectory = this->getDirectory(toPath, true);

    if (fromDirectory == nullptr) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 1;
    }

    if (toDirectory == nullptr) {
        std::cerr << "PATH NOT FOUND" << std::endl;
        return 1;
    }

    std::string fromFilename = fromPath.substr(fromPath.find_last_of('/') + 1);
    std::string toFilename = toPath.substr(toPath.find_last_of('/') + 1);
    std::shared_ptr<INode> inputFile = fromDirectory->getItem(fromFilename);
    if (inputFile == nullptr || inputFile->inode->isDirectory) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 2;
    }

    if (toDirectory->getItem(toFilename) != nullptr) {
        std::cerr << "FILE EXISTS" << std::endl;
        return 3;
    }

    fromDirectory->removeItem(fromFilename);
    toDirectory->addItem(toFilename, inputFile);

    std::cout << "OK" << std::endl;
    return 0;
}

int System::removeFile(const std::string &from) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string fromPath = getRealPath(from);
    std::shared_ptr<Directory> fromDirectory = this->getDirectory(fromPath, true);

    if (fromDirectory == nullptr) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 1;
    }

    std::string fromFilename = fromPath.substr(fromPath.find_last_of('/') + 1);
    std::shared_ptr<INode> inputFile = fromDirectory->getItem(fromFilename);
    if (inputFile == nullptr || inputFile->inode->isDirectory) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 2;
    }

    fromDirectory->removeItem(fromFilename);
    inputFile->inode->references--;
    if (inputFile->inode->references <= 0) {
        inputFile->truncate(this->fileSystem);
        this->fileSystem->removeInode(inputFile->inode);
    } else {
        this->fileSystem->saveInode(inputFile->inode.get());
    }

    std::cout << "OK" << std::endl;
    return 0;
}

int System::hardLink(const std::string &from, const std::string &to) {
    int status = this->checkLoaded();
    if (status != 0) { return status; }

    std::string fromPath = getRealPath(from);
    std::shared_ptr<Directory> fromDirectory = this->getDirectory(fromPath, true);

    std::string toPath = getRealPath(to);
    std::shared_ptr<Directory> toDirectory = this->getDirectory(toPath, true);

    if (fromDirectory == nullptr) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 1;
    }

    if (toDirectory == nullptr) {
        std::cerr << "PATH NOT FOUND" << std::endl;
        return 1;
    }

    std::string fromFilename = fromPath.substr(fromPath.find_last_of('/') + 1);
    std::string toFilename = toPath.substr(toPath.find_last_of('/') + 1);
    std::shared_ptr<INode> inputFile = fromDirectory->getItem(fromFilename);
    if (inputFile == nullptr || inputFile->inode->isDirectory) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return 2;
    }

    if (toDirectory->getItem(toFilename) != nullptr) {
        std::cerr << "FILE EXISTS" << std::endl;
        return 3;
    }

    toDirectory->addItem(toFilename, inputFile);
    inputFile->inode->references++;
    this->fileSystem->saveInode(inputFile->inode.get());

    std::cout << "OK" << std::endl;
    return 0;
}

int System::format(unsigned long size) {
    this->fileSystem->format(size);
    this->fileSystem->load();
    this->loaded = true;

    std::cout << "OK" << std::endl;
    return 0;
}
