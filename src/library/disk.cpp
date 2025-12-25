#include "disk.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

Disk::Disk() {
    file_descriptor_ = -1;
    blocks_ = 0;           
    reads_  = 0;
    writes_ = 0;
}

Disk::~Disk() {
    close();
}

bool Disk::disk_sanity_check(size_t block, const char *data) {
    if(block < 0 || block >= blocks_) {
        printf("Invalid block num %ld\n", block);
        return false;
    }
    if(data == nullptr) {
        printf("null data pointer\n");
        return false;
    }
    return true;
}

bool Disk::open(const char *path, size_t blocks) {
    if(!path || blocks == 0)
        return false;
    
    file_descriptor_ = ::open(path, O_RDWR|O_CREAT, 0644);
    if(file_descriptor_ < 0) {
        printf("Failed to open %s - %s\n", path, strerror(errno));
        return false;
    }

    int ret = ftruncate(file_descriptor_, blocks * BLOCK_SIZE);
    if(ret < 0) {
        printf("Failed to ftruncate - %s\n", strerror(errno));
        return false;
    }

    blocks_ = blocks;
    reads_  = 0;
    writes_ = 0;

    return true;
}

void Disk::close() {
    if(file_descriptor_ > 0) {
        printf("%lu disk block reads\n", reads_);
    	printf("%lu disk block writes\n", writes_);
    	::close(file_descriptor_);
    	file_descriptor_ = 0;
    }
}

ssize_t Disk::read(size_t block, char *data) {
    if(not disk_sanity_check(block, data)) {
        return false;
    }

    // Calculate the byte offset for the specified block
    off_t offset = block * BLOCK_SIZE;
    if(lseek(file_descriptor_, offset, SEEK_SET) < 0) {
        printf("Error in lseek - %s\n", strerror(errno));
        return -1;
    }

    // Reading from block to data buffer (must be BLOCK_SIZE)
    ssize_t bytes = ::read(file_descriptor_, data, BLOCK_SIZE);
    if(bytes != BLOCK_SIZE) {
        printf("Error in read - %s\n", strerror(errno));
        return -1;
    }
    reads_++;
    return bytes;
}

ssize_t Disk::write(size_t block, char *data) {
    if(not disk_sanity_check(block, data)) {
        return false;
    }

    off_t offset = block * BLOCK_SIZE;
    if(lseek(file_descriptor_, offset, SEEK_SET) < 0) {
        printf("Error in lseek - %s\n", strerror(errno));
        return -1;
    }

    ssize_t bytes = ::write(file_descriptor_, data, BLOCK_SIZE);
    if(bytes != BLOCK_SIZE) {
        printf("Error in write - %s\n", strerror(errno));
        return -1;
    }
    writes_++;
    return bytes;
}
