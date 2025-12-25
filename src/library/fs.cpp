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

ssize_t FileSystem::allocBlock() {
    if(!free_blocks_)
        return -1;
    // find first free block
    for(int i = 0; i < meta_data_.blocks; ++i) {
        if(!free_blocks_[i]) {
            free_blocks_[i] = true;
            return (ssize_t)i;
        }
    }
    return -1;
}

void FileSystem::debug(Disk& disk) {
// SuperBlock:
//     magic number is valid
//     20 blocks
//     2 inode blocks
//     256 inodes
// Inode 2:
//     size: 27160 bytes
//     direct blocks: 4 5 6 7 8
//     indirect block: 9
//     indirect data blocks: 13 14
// Inode 3:
//     size: 9546 bytes
//     direct blocks: 10 11 12
// 4 disk block reads
// 0 disk block writes
    Block block;

    /* Read SuperBlock */
    disk.read(0, block.data);

    printf("SuperBlock:\n");
    printf("    %u blocks\n"         , block.super.blocks);
    printf("    %u inode blocks\n"   , block.super.inode_blocks);
    printf("    %u inodes\n"         , block.super.inodes);

    /* Read Inodes */
    size_t inodeBlocksNum = block.super.inode_blocks;
    Block data_block = {0};
    for(int blockIdx = 1; blockIdx <= inodeBlocksNum; ++blockIdx) {
        if(disk.read(blockIdx, data_block.data) != Disk::BLOCK_SIZE) {
            printf("Failed to read block.\n");
            return;
        }
        for(int inodeIdx = 0; inodeIdx < INODES_PER_BLOCK; ++inodeIdx) {
            Inode* inode = &data_block.inodes[inodeIdx];
            size_t total_inode_num = (blockIdx - 1) * INODES_PER_BLOCK + inodeIdx;
            if(total_inode_num >= block.super.inodes)
                break;
            
            if(inode->valid == 1) {
                
            } 




        }





    }






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
    disk_ = &disk;
    free_blocks_ = (bool*)calloc(numBlocks, sizeof(bool));

    size_t numInodes   = numBlocks / 10; /*use 10% of total Blocks*/
    if(numInodes < INODES_PER_BLOCK) numInodes = INODES_PER_BLOCK;
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
    Block block = {0};
    // read super block
    if(disk.read(0, block.data) != Disk::BLOCK_SIZE) {
        return false;
    }
    if(block.super.magic_number != MAGIC_NUMBER) {
        return false;
    }
    if(block.super.blocks != disk.getBlockNum()) {
        return false;
    }
    meta_data_ = block.super;
    disk_ = &disk;
    if(free_blocks_) {
        free(free_blocks_);
        free_blocks_ = nullptr;
    }
    free_blocks_ = (bool*)calloc(disk.getBlockNum(), sizeof(bool));
    free_blocks_[0] = true;
    // inode block used
    for(int i = 0; i < meta_data_.inode_blocks; ++i) {
        free_blocks_[1 + i] = true;
    }

    return true;
}

void FileSystem::unmount() {
    if(free_blocks_) {
        free(free_blocks_);
        free_blocks_ = nullptr;
    }
    disk_ = nullptr;
    meta_data_ = (SuperBlock){0};
}

ssize_t FileSystem::create() {
    if(!disk_ || !free_blocks_) {
        return -1;
    }

    // find a free inode
    for(int inode = 0; inode < meta_data_.inodes; ++inode) {
        size_t blockIdx = 1 + (inode / INODES_PER_BLOCK);
        size_t offset   = inode % INODES_PER_BLOCK;

        // Read the block containing this inode
        Block block = {0};
        if(disk_->read(blockIdx, block.data) != Disk::BLOCK_SIZE) {
            /*文件系统应该具备部分故障隔离能力，单个inode块的问题不应该导致整个文件创建操作失败。
            跳过损坏的inode块可以让文件系统继续使用其他正常的inode块*/
            continue;
        }
        // Check if this inode is free
        if(block.inodes[offset].valid == 1) {
            continue;
        }
        block.inodes[offset].valid = 1;
        block.inodes[offset].size = 0;
        // clear all pointers
        for(int i = 0; i < POINTERS_PER_INODE; ++i) {
            block.inodes[offset].direct[i] = 0;
        }
        block.inodes[offset].indirect = 0;

        // write the updated block back to disk
        if(disk_->write(blockIdx, block.data) != Disk::BLOCK_SIZE) {
            return -1;
        }
        return (ssize_t)inode;
    }
    return -1;
}

bool FileSystem::remove(size_t inode_number) {
    if(!disk_ || !free_blocks_) {
        return false;
    }
    if(inode_number >= meta_data_.inodes) {
        return false;
    }
    size_t blockIdx = 1 + (inode_number / INODES_PER_BLOCK);
    size_t offset   = inode_number % INODES_PER_BLOCK;
    Block block = {0};
    if(disk_->read(blockIdx, block.data) != Disk::BLOCK_SIZE) {
        return false;
    }
    // Check if this inode is free
    Inode* inode = &block.inodes[offset];
    if(inode->valid != 1) {
        return false;
    }
    // Release direct blocks
    for(int i = 0; i < POINTERS_PER_INODE; ++i) {
        if(inode->direct[i] != 0) {
            if(inode->direct[i] < disk_->getBlockNum()) {
                free_blocks_[inode->direct[i]] = 0;
            }else {
                printf("Unexpected error in direct block\n");
            }
        }
        inode->direct[i] = 0;
    }
    // Release indirect block
    if(inode->indirect != 0) {
        Block ind_block = {0};
        if(disk_->read(inode->indirect, ind_block.data) != Disk::BLOCK_SIZE) {
            return false;
        }
        for(int i = 0; i < POINTERS_PER_BLOCK; ++i) {
            int blockId = ind_block.pointers[i];
            if(blockId != 0 && blockId < disk_->getBlockNum()) {
                // mark block as free in bit map
                free_blocks_[blockId] = 0;
            }
            ind_block.pointers[i] = 0;
        }
        if(inode->indirect < disk_->getBlockNum()) {
            free_blocks_[inode->indirect] = 0;
        }
        inode->indirect = 0;
    }
    // mark inode as free
    inode->valid = 0;
    inode->size  = 0;

    // write the updated block back to disk
    if(disk_->write(blockIdx, block.data) != Disk::BLOCK_SIZE) {
        return false;
    }

    return true;
}

ssize_t FileSystem::stat(size_t inode_number) {
    if(!disk_ || !free_blocks_) {
        return -1;
    }
    if(inode_number >= meta_data_.inodes) {
        return -1;
    }
    size_t blockIdx = 1 + (inode_number / INODES_PER_BLOCK);
    size_t offset   = inode_number % INODES_PER_BLOCK;
    Block block = {0};
    if(disk_->read(blockIdx, block.data) != Disk::BLOCK_SIZE) {
        return -1;
    }
    Inode* inode = &block.inodes[offset];
    if(inode->valid != 1) {
        return -1;
    }

    return (ssize_t)inode->size;
}

ssize_t FileSystem::read(size_t inode_number, char *data, size_t length, size_t offset) {
    if(!disk_ || !free_blocks_ || !data) {
        return -1;
    }
    if(inode_number >= meta_data_.inodes) {
        return -1;
    }
    size_t blockIdx = 1 + (inode_number / INODES_PER_BLOCK);
    size_t offsetInBlock   = inode_number % INODES_PER_BLOCK;
    Block block = {0};
    if(disk_->read(blockIdx, block.data) != Disk::BLOCK_SIZE) {
        return -1;
    }
    Inode* inode = &block.inodes[offset];
    if(inode->valid != 1) {
        return -1;
    }

    // Calculate total bytes to read
    size_t total_bytes = length;
    if(offset + length > inode->size) {
        total_bytes = inode->size - offset;
    }
    if(total_bytes <= 0) {
        return 0;
    }

    size_t bytes_read = 0;
    // Calculate starting block and offset within blocks of this file
    size_t current_block_idx = offset / Disk::BLOCK_SIZE;
    size_t block_offset      = offset % Disk::BLOCK_SIZE;

    Block data_block = {0};
    while(bytes_read < total_bytes && current_block_idx < POINTERS_PER_INODE) {
        size_t bIndex = inode->direct[current_block_idx];
        if(bIndex != 0) {
            // read the data block
            if(disk_->read(bIndex, data_block.data) != Disk::BLOCK_SIZE) {
                return -1;
            }
            // Calculate bytes to copy from this block
            size_t bytes_to_copy = Disk::BLOCK_SIZE - block_offset;
            if(bytes_to_copy > total_bytes - bytes_read) {
                bytes_to_copy = total_bytes - bytes_read;
            }
            // Copy data to buffer
            memcpy(data + bytes_read, data_block.data + block_offset, bytes_to_copy);
            bytes_read += bytes_to_copy;
        }

        current_block_idx++;
        block_offset = 0; /*set offset to 0*/
    }
    
    // Read data from indirect block if needed
    if(bytes_read < total_bytes && inode->indirect != 0) {
        data_block = {0};
        Block indirect_block = {0};
        if(disk_->read(inode->indirect, indirect_block.data) != Disk::BLOCK_SIZE) {
            return -1;
        }
        // NOTE: indirect_idx = 0 here is error
        size_t indirect_idx = current_block_idx - POINTERS_PER_INODE;
        while(bytes_read < total_bytes && indirect_idx < POINTERS_PER_BLOCK) {
            int bIndex = indirect_block.pointers[indirect_idx];
            if(bIndex != 0) {
                // Read the data block
                if(disk_->read(bIndex, data_block.data) != Disk::BLOCK_SIZE) {
                    return -1;
                }
                // Calculate bytes to copy from this block
                size_t bytes_to_copy = Disk::BLOCK_SIZE - block_offset;
                if(bytes_to_copy > total_bytes - bytes_read) {
                    bytes_to_copy = total_bytes - bytes_read;
                }
                // Copy data to buffer
                memcpy(data + bytes_read, data_block.data + block_offset, bytes_to_copy);
                bytes_read += bytes_to_copy;
            }
            indirect_idx++;
            block_offset = 0;          
        }
    }
    if(bytes_read != total_bytes) {
        printf("FS read: bytes unmatched!\n");
    }
    
    return (ssize_t)bytes_read;
}

ssize_t FileSystem::write(size_t inode_number, char *data, size_t length, size_t offset) {
    if(!disk_ || !free_blocks_ || !data) {
        return -1;
    }
    if(inode_number >= meta_data_.inodes) {
        return -1;
    }
    size_t blockIdx = 1 + (inode_number / INODES_PER_BLOCK);
    size_t offsetInBlock   = inode_number % INODES_PER_BLOCK;
    Block block = {0};
    if(disk_->read(blockIdx, block.data) != Disk::BLOCK_SIZE) {
        return -1;
    }
    Inode* inode = &block.inodes[offset];
    if(inode->valid != 1) {
        return -1;
    }

    size_t current_block_idx = offset / Disk::BLOCK_SIZE;
    size_t block_offset      = offset % Disk::BLOCK_SIZE;

    size_t bytes_written = 0;
    Block data_block = {0};
    // direct blocks
    while(bytes_written < length && current_block_idx < POINTERS_PER_INODE) {
        int bIndex = inode->direct[current_block_idx];
        if(bIndex == 0) {
            // alloc new block
            ssize_t new_block = allocBlock();
            if(new_block == -1) {
                return -1;
            }
            inode->direct[current_block_idx] = (uint32_t)new_block;
        }
        // Read existing data block if we're writing to a non-empty block
        if(block_offset > 0) {
            if(disk_->read(inode->direct[current_block_idx], data_block.data) != Disk::BLOCK_SIZE) {
                return -1;
            }
        }
        // Calculate bytes to copy to this block
        size_t bytes_to_copy = Disk::BLOCK_SIZE - block_offset;
        if(bytes_to_copy > length - bytes_written) {
            bytes_to_copy = length - bytes_written;
        }
        // copy data to buffer
        memcpy(data_block.data + block_offset, data + bytes_written, bytes_to_copy);
        // Write data block back to disk
        if(disk_->write(bIndex, data_block.data) != Disk::BLOCK_SIZE) {
            return -1;
        }
        bytes_written += bytes_to_copy;
        current_block_idx++;
        block_offset = 0;
    }

    // Handle indirect blocks if needed
    data_block = {0};
    if(bytes_written < length) {
        Block indirect_block = {0};
        size_t indirect_idx = current_block_idx - POINTERS_PER_BLOCK;
        // Allocate indirect block if needed
        if(inode->indirect == 0) {
            ssize_t new_block = allocBlock();
            if(new_block == -1)
                return -1;
            inode->indirect = new_block;
            memset(indirect_block.data, 0, Disk::BLOCK_SIZE);
            if(disk_->write(inode->indirect, indirect_block.data) != Disk::BLOCK_SIZE) {
                return -1;
            }
        }else {
            // indirect block exist, read it
            if(disk_->read(inode->indirect, indirect_block.data) != Disk::BLOCK_SIZE) {
                return -1;
            }
        }

        // Write data to indirect blocks
        while(bytes_written < length && indirect_idx < POINTERS_PER_BLOCK) {
            int bIndex = indirect_block.pointers[indirect_idx];
            if(bIndex == 0) {
                ssize_t new_block = allocBlock();
                if(new_block == -1)
                    return -1;
                indirect_block.pointers[indirect_idx] = (uint32_t)new_block;
                // Write updated indirect block
                if(disk_->write(inode->indirect, indirect_block.data) != Disk::BLOCK_SIZE) {
                    return -1;
                }
            }

            // Read existing data block if we're writing to a non-empty block
            if(block_offset > 0) {
                if(disk_->read(indirect_block.pointers[indirect_idx], data_block.data) != Disk::BLOCK_SIZE) {
                    return -1;
                }
            }
            memset(data_block.data, 0, Disk::BLOCK_SIZE);

            size_t bytes_to_copy = Disk::BLOCK_SIZE - block_offset;
            if(bytes_to_copy > length - bytes_written) {
                bytes_to_copy = length - bytes_written;
            }

            memcpy(data_block.data + block_offset, data + bytes_written, bytes_to_copy);

            // write to disk
            if(disk_->write(indirect_block.pointers[indirect_idx], data_block.data) != Disk::BLOCK_SIZE) {
                return -1;
            }
            bytes_written += bytes_to_copy;
            indirect_idx++;
            block_offset = 0;
        }
    }
    if(offset + length > inode->size) {
        inode->size = offset + length;
        if(disk_->write(blockIdx, block.data) != Disk::BLOCK_SIZE) {
            return -1;
        }
    }

    return (ssize_t)bytes_written;
}