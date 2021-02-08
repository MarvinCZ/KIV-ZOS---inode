#pragma once

#include "structs.hpp"

const int32_t ID_ITEM_FREE = 0;
const int32_t INODE_SIZE = sizeof(pseudo_inode);
const int32_t CLUSTER_SIZE = 2048;
//const int32_t CLUSTER_SIZE = 512;
//const int32_t CLUSTER_SIZE = 128;
const int32_t SUPERBLOCK_SIZE = sizeof(superblock);
const int32_t CLUSTER_SIZE_PER_INODE_SIZE = 128;
const int32_t LINKS_PER_CLUSTER = CLUSTER_SIZE / sizeof(int32_t);
const int32_t MAX_FILE_SIZE = (5 + LINKS_PER_CLUSTER + LINKS_PER_CLUSTER*LINKS_PER_CLUSTER) * CLUSTER_SIZE;
