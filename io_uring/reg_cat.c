#include <fcntl.h>
#include <limits.h>
#include <linux/fs.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#define BLOCK_SZ 4096

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
    if (S_ISBLK(st.st_mode)) { // block devices
        unsigned long long nbytes;
        if (ioctl(fd, BLKGETSIZE64, &nbytes) != 0) {
            perror("ioctl");
            return 5;
        }
        return nbytes;
    } else if (S_ISREG(st.st_mode)) { // regular files
        return st.st_size;
    }

    return -1;
}

/*
 * We're using buffered output here to be efficient.
 */
void print_to_console(char *buf, unsigned long len) {
    fwrite(buf, 1, len, stdout);
}

int read_and_print_file(char *filename) {
    struct iovec iovecs[UIO_MAXIOV];
    char *buf;

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 3;
    }

    off_t file_sz = get_file_size(fd);
    if (file_sz == -1) {
        return 6;
    }

    /* buffer to hold the file content read by readv
     * Allocate one large buffer for all blocks we'll read in batches
     * This eliminates multiple posix_memalign calls and reduces fragmentation
     * add 1 for '\0' termination
     */
    off_t buf_size = (file_sz > (off_t)UIO_MAXIOV * (off_t)BLOCK_SZ)
                         ? (off_t)UIO_MAXIOV * (off_t)BLOCK_SZ + 1
                         : file_sz + 1;
    if (posix_memalign((void **)&buf, BLOCK_SZ, buf_size)) {
        perror("posix_memalign");
        return 7;
    }

    off_t bytes_remaining = file_sz;
    off_t blocks = (off_t)file_sz / BLOCK_SZ;
    if (file_sz % BLOCK_SZ)
        blocks++;

    /*
     *  each `iovec` is responsible for a 4KB block of data to read.
     * The `iovecs` array will be passed to `readv()` as an argument.
     */
    int cnt_blocks = 0;
    while (bytes_remaining) {
        int batch_size = (blocks - cnt_blocks > UIO_MAXIOV)
                             ? UIO_MAXIOV
                             : (blocks - cnt_blocks);

        off_t batch_bytes_read = 0;
        for (int i = 0; i < batch_size; i++) {
            off_t bytes_to_read =
                (bytes_remaining > BLOCK_SZ) ? BLOCK_SZ : bytes_remaining;
            iovecs[i].iov_base = buf + i * BLOCK_SZ;
            iovecs[i].iov_len = bytes_to_read;
            bytes_remaining -= bytes_to_read;
            batch_bytes_read += bytes_to_read;
        }

        int ret = readv(fd, iovecs, batch_size);
        if (ret < 0) {
            perror("readv");
            free(buf);
            return 8;
        }

        buf[batch_bytes_read] = '\0';
        cnt_blocks += batch_size;

        ret = writev(1, iovecs, batch_size);
        if (ret < 0) {
            perror("writev");
            free(buf);
            return 9;
        }
    }
    free(buf);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (read_and_print_file(argv[i])) {
            fprintf(stderr, "Error reading file\n");
            return 2;
        }
    }

    return 0;
}
