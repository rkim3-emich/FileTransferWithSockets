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
	//Engage WSA startup (Required for windows)
	WSADATA wsaData;
	int start = WSAStartup(MAKEWORD(2, 2), &wsaData);

	//Create socket with ipv4
	SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int new_socket;

	//Create address (for ip)
	struct sockaddr_in address;
	int addrlen = sizeof(address);

	//Option for socket settings
	char* opt = new char('1');

	//Create buffer for sending and receiving
	char buffer[1024] = { 0 };

	//Message sent on connection to client
	const char* hello = "Hello from server";

	//Check if WSA startup succeeded
	if (start != 0) {
		perror("WSAStartup Failure");
		exit(EXIT_FAILURE);
	}
	//Check socket creation flag
	if (!sockfd) {
		perror("Socket Creation Failure");
		exit(EXIT_FAILURE);
	}
	//Set socket options for address
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, opt, 1)) {
		perror("Socket Option Failure");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	InetPtonW(AF_INET, ip, &address.sin_addr.s_addr);
	address.sin_port = htons(PORT);

	//Attempt socket bind
	if (bind(sockfd, (struct sockaddr*) & address, sizeof(address)) < 0) {
		perror("Binding Error");
		exit(EXIT_FAILURE);
	}
	//Attemp to listen for connections
	if (listen(sockfd, 3) < 0) {
		perror("Listening Error");
		exit(EXIT_FAILURE);
	}
	//Accept incoming connection
	std::cout << "Server created and listening on " << (PCSTR) ip << ":" << PORT << "\n";
	if ((new_socket = accept(sockfd, (struct sockaddr*) & address, &addrlen)) < 0) {
		perror("Accept Error");
		exit(EXIT_FAILURE);
	}
	bool closed = false;

	//Receive incoming welcome message from client
	recv(new_socket, buffer, sizeof(buffer), 0);
	std::cout << "Bytes received: " << buffer << "\n";

	//Begin instruction loop
	do {
		//clear buffer
		memset(&buffer[0], 0, sizeof(buffer));

		//Get instruction from command line
		std::string command;
		getline(std::cin, command);

		//Parse commands
		if (command == "QUIT") {
			//Quit
			closesocket(new_socket);
			closed = true;
		}
		//Check if command is empty
		else if (command != "") {
			//Send command to client
			send(new_socket, command.c_str(), strlen(command.c_str()), 0);

			//Find instruction
			std::string instruction = command.substr(0, command.find(" "));
			if (instruction.compare("DUMPT") == 0) {
				//Retrieve text file from client system

				//Get length of text file from client
				recv(new_socket, buffer, sizeof(buffer), 0);
				long int num = strtol(buffer, NULL, 10);

				//Check if file size is not 0
				if (num != 0) {
					//clear buffer
					memset(&buffer[0], 0, sizeof(buffer));

					//initialize string that will be appended to
					std::string message = "";
					int receivedBytes = 0;
					//Loop while still bytes to receive
					while (receivedBytes < num) {
						//Receive text and add to string
						recv(new_socket, buffer, sizeof(buffer), 0);
						message += buffer;
						receivedBytes = strlen(message.c_str());
						//clear buffer
						memset(&buffer[0], 0, sizeof(buffer));
					}
					//Display message
					std::cout << message << "\n";
				}
				else {
					//Display contents of buffer
					std::cout << buffer << "\n";
				}
			}
			else if (instruction.compare("DUMPB") == 0) {
				//Retrieve binary file from client
				
				//receive size of file from client
				recv(new_socket, buffer, sizeof(buffer), 0);
				long int num = strtol(buffer, NULL, 10);

				//Check if file size is 0
				if (num != 0) {
					//clear buffer
					memset(&buffer[0], 0, sizeof(buffer));

					//Open write stream
					std::filebuf f;
					f.open("test.bin", std::ios::out | std::ios::binary);
					std::ostream os(&f);
					
					//loop while still bytes to receive
					int receivedBytes = num;
					while (receivedBytes > 0) {
						//clear buffer
						memset(&buffer[0], 0, sizeof(buffer));
						//receive and write bytes
						int bytes = recv(new_socket, buffer, sizeof(buffer), 0);
						os.write(buffer, bytes);
						receivedBytes -= bytes;
					}
					//close file
					f.close();
				}
				else {
					std::cout << buffer << "\n";
				}
			}
			else if (instruction.compare("HELP") == 0) {
				//Display help message
				std::cout << "INSTRUCTIONS:\n";
				std::cout << "Request client send a text file in its folder: DUMPT [File name]\n";
				std::cout << "Request client send some form of binary file in its folder: DUMPB [File name]\n";
				std::cout << "Shutdown console: QUIT\n";
			}
			else {
				//display invalid instruction message
				std::cout << "INSTRUCTION NOT FOUND\nENTER HELP FOR INSTRUCTIONS\n";
			}
			
			command = "";
		}
	} while (!closed);
	
	WSACleanup();	
	
	return EXIT_SUCCESS;
}