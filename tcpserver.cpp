#include "tcpserver.h"
#include <iostream>
#include <memory.h>
#include <chrono>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

TCPServer::TCPServer(std::string interfaceIP, uint16_t localPort) : m_TxCounter(0), m_ListenSocket(-1)
{
	// Start thread
	m_Running = true;
	m_WorkerThread = std::thread(&TCPServer::WorkerThread, this, interfaceIP, localPort);
}

TCPServer::~TCPServer()
{
	// kill thread
	m_Running = false;
	if (m_WorkerThread.joinable()) m_WorkerThread.join();

	// close socket
	if (m_ListenSocket)
	{
		close(m_ListenSocket);
		m_ListenSocket = -1;
	}
}

void TCPServer::SendData(uint8_t* buffer, uint32_t size)
{
	std::lock_guard<std::mutex> lock(m_FrameQueueMutex);

	ImageBuffer newBuffer;
	newBuffer.ImagePtr = new uint8_t[size];
	newBuffer.Size = size;
	memcpy(newBuffer.ImagePtr, buffer, size);

	m_FrameQueue.push(newBuffer);
}

void TCPServer::WorkerThread(std::string interfaceIP, uint16_t localPort)
{
	// Create Listening Socket
	m_ListenSocket = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_ListenSocket == -1)
	{
		std::cout << "socket create error" << std::endl;
		return;
	}

	// Bind to Local Address
	sockaddr_in localaddr;
	memset((char *)&localaddr, 0, sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(localPort); // listening port
	localaddr.sin_addr.s_addr = inet_addr(interfaceIP.c_str());

	if (bind(m_ListenSocket, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0)
	{
		std::cout << "bind failed" << std::endl;
		return;
	}

	// set to listen
	if (listen(m_ListenSocket, 3) < 0)
	{
		std::cout << "listen failed" << std::endl;
		return;
	}

	std::cout << "Server Started on port: " << localPort<<  std::endl;

	while (m_Running)
	{
		// Wait for client
		sockaddr_in clientaddr;
		socklen_t addrLen = sizeof(clientaddr);
		int clientSock = (int)accept(m_ListenSocket, (sockaddr*)&clientaddr, &addrLen);

		if (clientSock >= 0)
		{
			// client connected, transfer data
			std::cout << "Connected from: " << inet_ntoa(clientaddr.sin_addr) << std::endl;
			do
			{
				// Send Frame
				ImageBuffer buffer{};
				{
					std::lock_guard<std::mutex> lock(m_FrameQueueMutex);
					if( !m_FrameQueue.empty() )
					{
						buffer = m_FrameQueue.front();
						m_FrameQueue.pop();	
					}	
					
				}
				if( buffer.ImagePtr != nullptr )
				{
					int snt = send(clientSock, (char*)buffer.ImagePtr, buffer.Size, MSG_NOSIGNAL); // MSG_NOSIGNAL - do not send SIGPIPE on close
					//std::cout << "Sending: " << buffer.Size << " bytes, sent: " << snt << " bytes" << std::endl;					
					if (snt <= 0) break; // error, kill connection
					m_TxCounter++;
				}
				
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // TBD, DATA RATE
			} while (1);
		}

		// client disconnected, error
		std::cout << "Disconnected" << std::endl << std::endl;
		close(clientSock);
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
}

