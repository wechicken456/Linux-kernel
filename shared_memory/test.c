#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

struct region {
    sem_t sem;
    char data[256];
};

int main() {
    const char *name = "/myshm";
    int fd = shm_open(name, O_CREAT | O_RDWR, 0600);
    ftruncate(fd, sizeof(struct region));
    struct region *r = (struct region *)mmap(
        NULL, sizeof(*r), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    shm_unlink(
        name); // still available to existing mappings until they munmap()

    sem_init(&r->sem, 1, 0); // pshared = 1
    pid_t p = fork();
    if (p == 0) {
        // child reads
        sem_wait(&r->sem);
        printf("child saw: %s\n", r->data);
        _exit(0);
    } else {
        // parent writes
        strcpy(r->data, "hello shared world");
        sem_post(&r->sem);
        wait(NULL);
        munmap(r, sizeof(*r));
    }
    return 0;
}
