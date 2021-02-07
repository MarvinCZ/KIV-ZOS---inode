#pragma once

#include <iostream>

struct superblock {
    int32_t disk_size;              //celkova velikost VFS
    int32_t inode_count;           //velikost clusteru
    int32_t cluster_count;          //pocet clusteru
    int32_t bitmapi_start_address;  //adresa pocatku bitmapy i-uzl�
    int32_t bitmap_start_address;   //adresa pocatku bitmapy datov�ch blok�
    int32_t inode_start_address;    //adresa pocatku  i-uzl�
    int32_t data_start_address;     //adresa pocatku datovych bloku
};


struct pseudo_inode {
    int32_t node_id;                //ID i-uzlu
    bool isDirectory;               //soubor, nebo adresar
    int8_t references;              //po�et odkaz� na i-uzel, pou��v� se pro hardlinky
    int32_t file_size;              //velikost souboru v bytech
    int32_t direct[5];              // 1.-5. p��m� odkaz na datov� bloky
    int32_t indirect1;              // 1. nep��m� odkaz (odkaz - datov� bloky)
    int32_t indirect2;              // 2. nep��m� odkaz (odkaz - odkaz - datov� bloky)
};


struct directory_item {
    int32_t inode;                   // inode odpov�daj�c� souboru
    char item_name[12];              //8+3 + /0 C/C++ ukoncovaci string znak
};
