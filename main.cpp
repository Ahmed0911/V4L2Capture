#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
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


	// set capture format
	// for available see: #v4l2-ctl -d /dev/video0 --list-formats-ext
	struct v4l2_format format;
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	format.fmt.pix.width = 640;
	format.fmt.pix.height = 480;	 
	if(ioctl(fd, VIDIOC_S_FMT, &format) < 0){
	    perror("VIDIOC_S_FMT");
	    exit(1);
	}

	// inform device about buffers
	struct v4l2_requestbuffers bufrequest;
	bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufrequest.memory = V4L2_MEMORY_MMAP;
	bufrequest.count = 1; // one buffer
	 
	if(ioctl(fd, VIDIOC_REQBUFS, &bufrequest) < 0){
	    perror("VIDIOC_REQBUFS");
	    exit(1);
	}

	// Alocate buffers
	struct v4l2_buffer bufferinfo;
	memset(&bufferinfo, 0, sizeof(bufferinfo));
	bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufferinfo.memory = V4L2_MEMORY_MMAP;
	bufferinfo.index = 0;
	 
	if(ioctl(fd, VIDIOC_QUERYBUF, &bufferinfo) < 0){
	    perror("VIDIOC_QUERYBUF");
	    exit(1);
	}

	void* buffer_start = mmap( NULL, bufferinfo.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, bufferinfo.m.offset);
	if(buffer_start == MAP_FAILED){
	    perror("mmap");
	    exit(1);
	}
	// clear frame
	memset(buffer_start, 0, bufferinfo.length);


	// START CAPTURE


	// END CAPTURE

	close(fd);
	printf("Device %s Closed\n", DEV);

	return EXIT_SUCCESS;
}
