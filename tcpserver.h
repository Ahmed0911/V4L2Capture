#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <queue>

struct ImageBuffer
{
	uint8_t* ImagePtr;
	uint32_t Size;
};

class TCPServer
{
public:
	TCPServer(std::string interfaceIP, uint16_t localPort);
	virtual ~TCPServer();

	void SendData(uint8_t* buffer, uint32_t size);

	// Statistics
	uint32_t m_TxCounter;

private:
	int m_ListenSocket;
	std::thread m_WorkerThread;
	bool m_Running;
	std::mutex m_FrameQueueMutex;

	// Data
	std::queue<ImageBuffer> m_FrameQueue;

	// Methods
	void WorkerThread(std::string interfaceIP, uint16_t localPort);
};

