/* 
 * very simple select example
 * 
 * Author:	Shawn Ostermann
 * Date:	Tue Mar  9, 1999
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>




int main(int argc, char *argv[])
{
    int ret;
    fd_set read_fds;

    /* turn off all bits in read_fds */
    FD_ZERO(&read_fds);

    while (1) {
		/* turn on the standard input bit */
		FD_SET(0,&read_fds);

		/* block until stdin has data */
		printf("Blocking until input is ready...\n");
		ret = select(FD_SETSIZE, &read_fds, NULL, NULL, NULL);
		if (ret == -1) {
			perror("select");
			exit(-1);
		}

		printf("Select says there are %d fds available for reading\n", ret);

		/* check each file descriptor */
		if (FD_ISSET(0,&read_fds)) {
			char buf[1024];
			printf("stdin has data ready!\n");
			/* throw it away */
			read(0,buf,sizeof(buf));
		}
    }
}

