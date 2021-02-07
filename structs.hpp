#pragma once

#include <iostream>

struct superblock {
    int32_t disk_size;              //celkova velikost VFS
    int32_t inode_count;           //velikost clusteru
    int32_t cluster_count;          //pocet clusteru
    int32_t bitmapi_start_address;  //adresa pocatku bitmapy i-uzlù
    int32_t bitmap_start_address;   //adresa pocatku bitmapy datových blokù
    int32_t inode_start_address;    //adresa pocatku  i-uzlù
    int32_t data_start_address;     //adresa pocatku datovych bloku
};


struct pseudo_inode {
    int32_t node_id;                //ID i-uzlu
    bool isDirectory;               //soubor, nebo adresar
    int8_t references;              //poèet odkazù na i-uzel, používá se pro hardlinky
    int32_t file_size;              //velikost souboru v bytech
    int32_t direct[5];              // 1.-5. pøímý odkaz na datové bloky
    int32_t indirect1;              // 1. nepøímý odkaz (odkaz - datové bloky)
    int32_t indirect2;              // 2. nepøímý odkaz (odkaz - odkaz - datové bloky)
};


struct directory_item {
    int32_t inode;                   // inode odpovídající souboru
    char item_name[12];              //8+3 + /0 C/C++ ukoncovaci string znak
};
