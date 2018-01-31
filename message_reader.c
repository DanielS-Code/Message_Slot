
#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	if (argc<3)
	{
		 printf("number of arguments to low\n");
		 exit(-1);
	}
    int file_desc;
    int ret_val;
    char buffer[MAX_MESSAGE_LEN+1]; //+1 in case message len is 128
    int msgSize;
    file_desc = open(argv[1], O_RDWR );
    if( file_desc < 0 )
    {
        perror("Can't open device file");
        exit(-1);
    }
    unsigned long channelID = (unsigned long) atoi(argv[2]);
	if(channelID==0)  //may be characters insted of number
	{
		printf("illegal channel id\n");
		close(file_desc);
		exit(-1);
	}
    ret_val = ioctl( file_desc, IOCTL_SET_ENC, channelID);
    if (ret_val<0)
    {
        perror("ioctl - channel set has failed");
        close(file_desc);
        exit(-1);
    }
    msgSize = read( file_desc, buffer, MAX_MESSAGE_LEN);
    if (msgSize<0)
    {
        perror("failed to read content");
        close(file_desc);
        exit(-1);
    }
    buffer[msgSize] = '\0';
    close(file_desc);
    printf("the message %s was read from channel %ld of file %s\n",buffer,channelID,argv[1]);
    return 0;
}
