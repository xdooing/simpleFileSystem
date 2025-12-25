#pragma once

#include <stdlib.h>

class Disk {
public:
    // number of bytes per block
    const static size_t BLOCK_SIZE = 4096;
public:
    Disk();
    ~Disk();

    bool open(const char* path, size_t nblocks);
    ssize_t read(size_t block, char *data);
    ssize_t write(size_t block, char *data);
    void close();
    bool disk_sanity_check(size_t block, const char *data);
    size_t getBlockNum() { return blocks_; }



private:
    int	    file_descriptor_;	/* File descriptor of disk image	*/
    size_t  blocks_;            /* Number of blocks in disk image	*/
    size_t  reads_;             /* Number of reads to disk image	*/
    size_t  writes_;            /* Number of writes to disk image	*/
};