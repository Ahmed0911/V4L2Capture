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
#define NUMOFBUFFERS 5

int main()
{	
	// Open device
	int fd = open(DEV, O_RDWR);
	if( fd < 0 ) 
	{
		perror("Error opening device");
		exit(1);
	}
	printf("Device %s Open\n", DEV);


	// Retrieve capabilities
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


	// Set capture format
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


	// Inform device about buffers to allocate
	struct v4l2_requestbuffers bufrequest;
	bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufrequest.memory = V4L2_MEMORY_MMAP;
	bufrequest.count = NUMOFBUFFERS; // Two buffers
	 
	if(ioctl(fd, VIDIOC_REQBUFS, &bufrequest) < 0){
	    perror("VIDIOC_REQBUFS");
	    exit(1);
	}

	void* imageBuffer[NUMOFBUFFERS];
	int imageBufferLength = 0;

	for(int bufidx = 0; bufidx != NUMOFBUFFERS; bufidx++ )
	{
		// Allocate buffers
		struct v4l2_buffer bufferinfo;
		memset(&bufferinfo, 0, sizeof(bufferinfo));
		bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		bufferinfo.memory = V4L2_MEMORY_MMAP;
		bufferinfo.index = bufidx;
		 
		if(ioctl(fd, VIDIOC_QUERYBUF, &bufferinfo) < 0)
		{
		    perror("VIDIOC_QUERYBUF");
		    exit(1);
		}

		// get device mapped pointer
		imageBuffer[bufidx] = mmap( NULL, bufferinfo.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, bufferinfo.m.offset);
		if(imageBuffer[bufidx] == MAP_FAILED)
		{
		    perror("mmap");
		    exit(1);
		}
		imageBufferLength = bufferinfo.length; // assume all buffer os same length, used only for unmap

		// Clear frame buffer
		memset(imageBuffer[bufidx], 0, bufferinfo.length);

		printf("Allocated Idx: %d, Size: %d\n", bufferinfo.index, bufferinfo.length);
	}



	// START CAPTURE

	// Queue First Buffers
	for(int bufidx = 0; bufidx != NUMOFBUFFERS; bufidx++ )
	{
		struct v4l2_buffer bufferinfo;
		memset(&bufferinfo, 0, sizeof(bufferinfo));
		bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		bufferinfo.memory = V4L2_MEMORY_MMAP;
		bufferinfo.index = bufidx;
		if(ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0)
		{
		    perror("VIDIOC_QBUF");
		    exit(1);
		}
	}

	// Activate streaming
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(fd, VIDIOC_STREAMON, &type) < 0)
	{
	    perror("VIDIOC_STREAMON");
	    exit(1);
	}	

	// Main Loop
	for(int i=0; i!=100; i++)
	{
		// Get Filled Buffer
		struct v4l2_buffer bufferinfo;
		memset(&bufferinfo, 0, sizeof(bufferinfo));
		bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		bufferinfo.memory = V4L2_MEMORY_MMAP;
		if(ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0) // index will be filled by ioctrl
		{
		    perror("VIDIOC_DQBUF");
		    exit(1);
		}

		// Frame retrieved, do something
		printf("Buffer: %d, Image size: %d, Sequence: %d\n", bufferinfo.index, bufferinfo.bytesused, bufferinfo.sequence);
		/*int jpgfile;
		if((jpgfile = open("myimage.jpeg", O_WRONLY | O_CREAT, 0660)) < 0){
		    perror("open");
		    exit(1);
		}
		write(jpgfile, buffer_start, bufferinfo.length);
		close(jpgfile);*/		

		// Queue next buffer (i.e. release retrieved buffer)
		// Index is the same as DQ buffer
		bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		bufferinfo.memory = V4L2_MEMORY_MMAP;
		if(ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0)
		{
		    perror("VIDIOC_QBUF");
		    exit(1);
		}
	}	

	// Deactivate streaming
	if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
	{
	    perror("VIDIOC_STREAMOFF");
	    exit(1);
	}

	// END CAPTURE


	// release buffers
	for(int bufidx = 0; bufidx != NUMOFBUFFERS; bufidx++ )
	{
		munmap(imageBuffer[bufidx], imageBufferLength);
	}

	close(fd);
	printf("Device %s Closed\n", DEV);

	return EXIT_SUCCESS;
}
