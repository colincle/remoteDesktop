#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdlib>

#include "../../../tools/ErrorWarning.hpp"
#include "../../../tools/DebugStatement.hpp"
#include "../../../include/DataTypes.hpp"
#include "../../../include/Colors.hpp"

class CloudflaredTunnel;

class Network
{
private:
    t_config config;
    int sock = -1;
    int client = -1;
    bool streaming = false;

    CloudflaredTunnel* tunnel = nullptr;

    void startListening();
    void makeTunnel();
    void stream();
    void killProcessOnPort(int port);

public:
	explicit Network(const t_config& config);
	void runServer();
};
