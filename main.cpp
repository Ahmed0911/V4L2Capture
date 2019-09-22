#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
//#include <errno.h>
//#include <sys/types.h>
//#include <sys/stat.h>

#define DEV "/dev/video0"

int main()
{	
	// open device
	int fd = open(DEV, O_RDWR);
	if( fd < 0 ) 
	{
		perror("Error opening device");
		exit(1);
	}

	printf("Device %s Open\n", DEV);

	// retrieve capabilities
	struct v4l2_capability cap;
	if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
	{
	    perror("VIDIOC_QUERYCAP");
	    exit(1);
	}

	// dump info
	printf("Driver: %s\n", cap.driver);
	printf("Card: %s\n", cap.card);
	printf("Bus: %s\n", cap.bus_info);
	printf("Version: %u.%u.%u\n", (cap.version >> 16) & 0xFF, (cap.version >> 8) & 0xFF, cap.version & 0xFF);
	printf("Capture Capability: %s\n", cap.capabilities & V4L2_CAP_VIDEO_CAPTURE ? "YES":"NO");
	printf("Streaming Capability: %s\n", cap.capabilities & V4L2_CAP_STREAMING ? "YES":"NO");


	close(fd);
	printf("Device %s Closed\n", DEV);

	return EXIT_SUCCESS;
}
