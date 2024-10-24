#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>

int DEBUG = 0;
#define INP_BUF_LEN 1024
#define RECEIVED_BUF_LEN 1024

char inp_buf[INP_BUF_LEN];
char received_buf[RECEIVED_BUF_LEN];

int main() {
	int fd = open("/dev/chardev", O_RDWR);
	if (fd == -1) {
		perror("Failed to open /dev/chardev");	
		exit(0);
	}
	memset(inp_buf, 0, INP_BUF_LEN);
	memset(received_buf, 0, RECEIVED_BUF_LEN);
	
	while (1) {
		// read inp from user
		fgets(inp_buf, INP_BUF_LEN, stdin);
		int l = strlen(inp_buf);
		inp_buf[l] = '\0';
		if (l == 2 && inp_buf[0] == 'q' && inp_buf[1] == '\n') break; 

		// write it to /dev/chardev
		write(fd, inp_buf, l);

		printf("Received: ");
		// print out response
		int ret = 1;
		while (ret > 0) {
			ret = read(fd, received_buf, RECEIVED_BUF_LEN);
			if (ret == -1) {
				perror("Failed to read from device");
				break;
			}
			printf("%s", received_buf);
		}
		puts("");
	}
}
