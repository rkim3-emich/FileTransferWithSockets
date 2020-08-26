#define WIN32_LEAN_AND_MEAN

#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <winsock.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 20
PCWSTR ip = L"127.0.0.1";

int main() {
	//WSA startup for windows
	WSADATA wsaData;
	int start = WSAStartup(MAKEWORD(2, 2), &wsaData);

	//Create socket
	SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	const char* hello = "Hello from client";

	//Check WSA startup was successful
	if (start != 0) {
		perror("WSAStartup Error");
		exit(EXIT_FAILURE);
	}

	//Check for socket creation errors
	if (sockfd < 0) {
		perror("Socket Creation Error");
		exit(EXIT_FAILURE);
	}

	//Set address information
	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);

	if (InetPtonW(AF_INET, ip, &address.sin_addr) <= 0) {
		perror("IP Resolution Error");
		exit(EXIT_FAILURE);
	}

	//Connect socket to server
	if (connect(sockfd, (struct sockaddr*) & address, sizeof(address)) != 0) {
		perror("Connection Failure");
		exit(EXIT_FAILURE);
	}

	//Send connection message to server
	std::cout << "CONNECTED\n";
	send(sockfd, hello, strlen(hello), 0);
	std::cout << "Message Sent\n";

	//Begin loop to receive messages from server
	int receive = 0;
	do {
		//Make sure the socket is not currently receiving
		if (receive != WSAEINPROGRESS) {
			//create buffer
			char buffer[1024] = { 0 };

			//Receive instruction from server
			receive = recv(sockfd, buffer, sizeof(buffer), 0);

			//Make sure instruction is not null
			if (buffer[0] != 0) {
				//Parse the instruction
				std::cout << buffer << "\n";
				std::string message = buffer;
				std::string instruction = message.substr(0, message.find(" "));

				if (instruction.compare("DUMPT") == 0) {
					//Send text file

					//Open read stream
					std::ifstream f(message.substr(message.find(" ") + 1, message.size() - 5).c_str());
					if (f.fail()) {
						//If the file is not found, tell the server
						send(sockfd, "FILE NOT FOUND", 14, 0);
					}
					else {
						struct stat stat_buf;
						
						std::string fileContents = "";
						std::string str;

						//Load file into memory
						while (std::getline(f, str)) {
							fileContents += "\n" + str;
						}
						f.close();

						//get the length of the file
						int length = strlen(fileContents.c_str());

						//Send the length of the file followed by the actual text
						send(sockfd, std::to_string(length).c_str(), sizeof(std::to_string(length).c_str()), 0);
						send(sockfd, fileContents.c_str(), (int)length, 0);
					}
				}
				else if (instruction.compare("DUMPB") == 0) {
					//Send binary file

					//Open a file stream to read 
					std::ifstream f(message.substr(message.find(" ") + 1, message.size() - 5).c_str(), std::ios::binary | std::ios::in);
					if (f.fail()) {
						send(sockfd, "FILE NOT FOUND", 14, 0);
					}
					else {
						//Go to the end of the file
						f.seekg(0, f.end);
						//Get the length
						int length = f.tellg();
						//Go back to the beginning of the file
						f.seekg(0, f.beg);

						//Clear the buffer and send the file length to the server
						memset(&buffer[0], 0, sizeof(buffer));
						send(sockfd, std::to_string(length).c_str(), strlen(std::to_string(length).c_str()), 0);
						
						//Loop while still bytes to send
						do {
							//Get the length of bytes to send
							size_t num = min(length, sizeof(buffer));
							//clear the buffer
							memset(&buffer[0], 0, sizeof(buffer));
							//Read part of the file into memory
							f.read(buffer, num);

							//Send bytes to server
							int buflen = num;
							while (buflen > 0) {
								int sent = send(sockfd, buffer, (int)buflen, 0);
								buflen -= sent;
							}

							length -= num;
						} while (length > 0);
					}
				}
			}
		}
	} while (receive > 0);

	return EXIT_SUCCESS;
}