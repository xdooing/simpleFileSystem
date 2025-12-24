#pragma once

#include <stdint.h>
#include "disk.h"

class FileSystem {
public:
    FileSystem();
    ~FileSystem();
    void debug(Disk& disk);
    bool format(Disk& disk);
    bool mount(Disk& disk);
    void unmount();
    ssize_t create();
    bool remove(size_t inode_number);
    ssize_t stat(size_t inode_number);
    ssize_t read(size_t inode_number, char *data, size_t length, size_t offset);
    ssize_t write(size_t inode_number, char *data, size_t length, size_t offset);
    ssize_t allocBlock();

private:
    const static uint32_t MAGIC_NUMBER       = 0xf0f03410;
    const static uint32_t INODES_PER_BLOCK   = 128;               /* Number of inodes per block */
    const static uint32_t POINTERS_PER_INODE = 5;                 /* Number of direct pointers per inode */
    const static uint32_t POINTERS_PER_BLOCK = 1024;              /* Number of pointers per block */

    struct SuperBlock {
        uint32_t    magic_number;                   /* File system magic number */
        uint32_t    blocks;                         /* Number of blocks in file system */
        uint32_t    inode_blocks;                   /* Number of blocks reserved for inodes */
        uint32_t    inodes;                         /* Number of inodes in file system */
    };

    struct Inode {
        uint32_t    valid;                          /* Whether or not inode is valid */
        uint32_t    size;                           /* Size of file */
        uint32_t    direct[POINTERS_PER_INODE];     /* Direct pointers */
        uint32_t    indirect;                       /* Indirect pointers */
    };

    union Block {
        SuperBlock  super;                          /* View block as superblock */
        Inode       inodes[INODES_PER_BLOCK];       /* View block as inode */
        uint32_t    pointers[POINTERS_PER_BLOCK];   /* View block as pointers */
        char        data[Disk::BLOCK_SIZE];               /* View block as data */
    };

    Disk* disk_;                          /* Disk file system is mounted on */
    bool* free_blocks_;                   /* Free block bitmap, true means been used*/
    SuperBlock meta_data_;  
};
