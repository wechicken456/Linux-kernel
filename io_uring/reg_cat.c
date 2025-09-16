#include <stdio.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <malloc.h>
#define BLOCK_SZ    4096

/*
 * return the size of the file given by the file descriptor `fd`  
 * Correctly handles block devices as well.
 */
off_t get_file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        return 4;
    }
    if (S_ISBLK(st.st_mode)) {      // block devices
        unsigned long long nbytes;
        if (ioctl(fd, BLKGETSIZE64, &nbytes) != 0) {
            perror("ioctl");
            return 5;
        }
        return nbytes;
    }
    else if (S_ISREG(st.st_mode)) {           // regular files
        return st.st_size;
    }

    return -1;
}

/*
 * We're using buffered output here to be efficient.
 */
void print_to_console(char *buf, unsigned long len) {
    while (len--) {
        fputc(*buf++, stdout);
    }
}

int read_and_print_file(char* filename) {
    struct iovec *iovecs;
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 3;
    }

    off_t file_sz = get_file_size(fd);
    if (file_sz == -1) {
        return 6;
    }
    off_t bytes_remaining = file_sz;
    off_t blocks = (off_t)file_sz / BLOCK_SZ;
    if (file_sz %  BLOCK_SZ) blocks++;
    iovecs = malloc(sizeof(struct iovec) * blocks);

    
    /*
     *  allocate an `iovec` for each 4KB block of data to read.
     * The `iovecs` array will be passed to `readv()` as an argument.
     */
    int cur_block = 0;
    while (bytes_remaining) {
        off_t bytes_to_read = (bytes_remaining > BLOCK_SZ) ? BLOCK_SZ : bytes_remaining;
        void *buf;
        if (posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ)) {
            perror("posix_memalign");
            return 7;
        }
        iovecs[cur_block].iov_base = buf;
        iovecs[cur_block].iov_len = bytes_to_read;
        cur_block++;
        bytes_remaining -= bytes_to_read;
    }

    int ret = readv(fd, iovecs, blocks);
    if (ret < 0) {
        perror("readv");
        return 8;
    }
    for (int i = 0 ; i < blocks; i++) {
        print_to_console(iovecs[i].iov_base, iovecs[i].iov_len);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    /* For each input file, call the read_and_print_file() function */
    for (int i = 1; i < argc; i++) {
        if (read_and_print_file(argv[i])) {
            fprintf(stderr, "Error reading file\n");
            return 2;
        }
    }

    return 0;
}