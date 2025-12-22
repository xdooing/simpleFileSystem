#include "fs.h"
#include <stdio.h>
#include <string.h>

FileSystem::FileSystem() {
    disk_ = nullptr;
    free_blocks_ = nullptr;
}

FileSystem::~FileSystem() {
    if(free_blocks_) {
        free(free_blocks_);
        free_blocks_ = nullptr;
    }
}

void FileSystem::debug(Disk& disk) {
    Block block;

    /* Read SuperBlock */
    disk.read(0, block.data);

    printf("SuperBlock:\n");
    printf("    %u blocks\n"         , block.super.blocks);
    printf("    %u inode blocks\n"   , block.super.inode_blocks);
    printf("    %u inodes\n"         , block.super.inodes);

    /* Read Inodes */
}

/**
 * Format Disk by doing the following:
 *  1. Write SuperBlock (with appropriate magic number, number of blocks,
 *     number of inode blocks, and number of inodes).
 *  2. Clear all remaining blocks.
 * Note: Do not format a mounted Disk!
 **/
bool FileSystem::format(Disk& disk) {
    Block block = {0};
    //1. write super block
    size_t numBlocks   = disk.getBlockNum();
    if(free_blocks_) {
        free(free_blocks_);
        free_blocks_ = nullptr;
    }
    free_blocks_ = (bool*)calloc(numBlocks, sizeof(bool));

    size_t numInodes   = numBlocks / 10; /*use 10% of total Blocks*/
    if(numInodes < 100) numInodes = 100;
    uint32_t numInodeBlocks = (numInodes + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;
    
    meta_data_.magic_number = MAGIC_NUMBER;
    meta_data_.blocks       = numBlocks;
    meta_data_.inode_blocks = numInodeBlocks;
    meta_data_.inodes       = numInodes;

    block.super = meta_data_;
    if(disk.write(0, block.data) != Disk::BLOCK_SIZE) {
        printf("Failed to write super block.\n");
        return false;
    }
    free_blocks_[0] = true;

    // 2. clear all inode table
    for(int i = 0; i < numInodeBlocks; ++i) {
        Block iBlock = {0};
        if(disk.write(i + 1, iBlock.data) != Disk::BLOCK_SIZE) {
            printf("Failed to write inode block.\n");
            return false;
        }
        free_blocks_[i + 1] = true;
    }
    // 3. write root dir inode (inode 0 in block 1)
    Block rootInodeBlock = {0};
    if(disk.read(1, rootInodeBlock.data) != Disk::BLOCK_SIZE) {
        printf("Failed to read rootInodeBlock.\n");
        return false;
    }
    rootInodeBlock.inodes[0].valid = 1;
    rootInodeBlock.inodes[0].size  = 0;
    if(disk.write(1, rootInodeBlock.data) != Disk::BLOCK_SIZE) {
        printf("Failed to write rootInodeBlock.\n");
        return false;
    }

    // 4.Clear all remaining blocks.
    for(int i = 1 + numInodeBlocks; i < numBlocks; ++i) {
        Block rmBlock = {0};
        if(disk.write(i, rmBlock.data) != Disk::BLOCK_SIZE) {
            printf("Failed to write rmBlock.\n");
            return false;
        }
    }
    
    return true;
}

bool FileSystem::mount(Disk& disk) {
    return true;
}

void FileSystem::unmount() {
}

ssize_t FileSystem::create() {
    return -1;
}

bool FileSystem::remove(size_t inode_number) {
    return true;
}

ssize_t FileSystem::stat(size_t inode_number) {
    return -1;
}

ssize_t FileSystem::read(size_t inode_number, char *data, size_t length, size_t offset) {
    return -1;
}

ssize_t FileSystem::write(size_t inode_number, char *data, size_t length, size_t offset) {
    return -1;
}