#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_NAME "/dev/pg_acc_gyro"

int main(int argc, char *argv[])
{
	int fd = 0;
	char read_buf[24];

	fd = open(DEV_NAME,O_RDWR);
	if(fd < 0)
	{
		printf("open %s error\n", DEV_NAME);
		return -1;
	}

	while(1) {
		printf("hello world.\n");
		read(fd, read_buf, 2);
		printf("buf=0x%x, 0x%x\n", read_buf[0], read_buf[1]);
		read(fd, read_buf, 3);
		printf("buf=0x%x, 0x%x, 0x%x\n", read_buf[0], read_buf[1], read_buf[2]);
		read(fd, read_buf, sizeof(read_buf)/sizeof(char));
		printf("buf=0x%x, 0x%x, 0x%x, 0x%x 0x%x, 0x%x, 0x%x, 0x%x 0x%x, 0x%x, 0x%x, 0x%x 0x%x, 0x%x, 0x%x, 0x%x 0x%x, 0x%x, 0x%x, 0x%x 0x%x, 0x%x, 0x%x, 0x%x\n", 
			read_buf[0], read_buf[1], read_buf[2], read_buf[3], read_buf[4], read_buf[5], read_buf[6], read_buf[7],
			read_buf[8], read_buf[9], read_buf[10], read_buf[11], read_buf[12], read_buf[13], read_buf[14], read_buf[15],
			read_buf[16], read_buf[17], read_buf[18], read_buf[19], read_buf[20], read_buf[21], read_buf[22], read_buf[23]);
		sleep(1);
	}
	
	close(fd);

	return 0;

}
