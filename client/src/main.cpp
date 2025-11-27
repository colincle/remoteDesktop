#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <QApplication>
#include "./TcpClient/TcpClient.hpp"
#include "../../tools/ErrorWarning.hpp"
#include "../../tools/DebugStatement.hpp"

void getIpPort(int argc, char* argv[], const char*& ip, int* port)
{
	if (argc != 3)
		error("Usage: " + std::string(argv[0]) + " <IP> <PORT>\n", argv[0]);

	ip = argv[1];
	*port = std::stoi(argv[2]);
}

int main(int argc, char* argv[])
{
	const char* ip;
	int port;
	getIpPort(argc, argv, ip, &port);

	QApplication app(argc, argv);
	TcpClient client(ip, port);
	client.run();

	return app.exec();
}
