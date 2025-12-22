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

bool Disk::open(const char *path, size_t blocks) {
    file_descriptor_ = ::open(path, O_RDWR|O_CREAT, 0600);
    if(file_descriptor_ < 0) {
        printf("Failed to open %s - %s\n", path, strerror(errno));
        return false;
    }

    int ret = ftruncate(file_descriptor_, blocks * BLOCK_SIZE);
    if(ret < 0) {
        printf("Failed to ftruncate - %s\n", strerror(errno));
        return false;
    }

    blocks  = blocks;
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
    return 0;
}

ssize_t Disk::write(size_t block, char *data) {
    return 0;
}

void Disk::disk_sanity_check(size_t block, const char *data) {
    
}
