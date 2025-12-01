#include "Network.hpp"
#include "../../../tools/DebugStatement.hpp"
#include "../Screenshot/Screenshot.hpp"
#include "../CloudflaredTunnel/CloudflaredTunnel.hpp"
#include <iostream>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstdio>
#include <memory>
#include <array>
#include <cstdlib>

Network::Network(t_config& config) : config(config) {}

void Network::runServer() {
	while (true) {
		killProcessOnPort(config.portStart);
		// Ensure the server socket is listening
		if (sock < 0) {
			startListening();
			makeTunnel();
		}
		std::cout << "Looking for client..." << std::endl;
		client = accept(sock, nullptr, nullptr);
		if (client < 0) {
			warning("accept failed: " + std::string(strerror(errno)), "Network");
			close(sock);  // close failed socket and retry
			sock = -1;
			continue;
		}
		DEBUG_OUT << "Client connected" << std::endl;

		Streaming = true;
		w = h = 0;

		// Write HTTP header once
		std::string header =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
			"Connection: keep-alive\r\n\r\n";
		send(client, header.c_str(), header.size(), 0);

		stream();

		close(client);  // close client on disconnect
		client = -1;
	}
}

void Network::startListening()
{
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(config.ip.c_str());

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		error("socket failed: " + std::string(strerror(errno)), "Network");

	bool bound = false;
	for (int port = config.portStart; port <= config.portEnd; port++) {
		addr.sin_port = htons(port);
		int opt = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
			config.port = port;
			bound = true;
			break;
		}
	}

	if (!bound)
		error("no available port", "Network");

	if (listen(sock, SOMAXCONN) < 0)
		error("listen failed: " + std::string(strerror(errno)), "Network");
}

void Network::makeTunnel() {
    if (!tunnel || !tunnel->isRunning()) {
        delete tunnel;
        tunnel = new CloudflaredTunnel(CLOUDFLARED_PATH, config.port, config.cloudflaredUrl);
        if (!tunnel->start())
            error("Failed to retrieve cloudflared tunnel.", "Network::launch");
    }

    DEBUG_OUT << std::endl << CYAN << "NETWORK" << std::endl
              << "=======" << RESET << std::endl << "Listening on "
              << config.ip << ":" << config.port << std::endl;
}

void Network::stream() {
	t_frame frame;

	while (Streaming) {
		int width = 0, height = 0;
		if (!Screenshot::newFrame(width, height, 90, frame))
			continue; // Nothing changed

		if (frame.fullFrame) {
			bool fullFlag = true;
			send(client, &fullFlag, sizeof(fullFlag), 0);

			int dataSize = static_cast<int>(frame.screen.size());
			send(client, &dataSize, sizeof(dataSize), 0);

			size_t sent = 0;
			while (sent < frame.screen.size()) {
				ssize_t n = send(client, frame.screen.data() + sent, frame.screen.size() - sent, 0);
				if (n <= 0) { Streaming = false; break; }
				sent += n;
			}
			if (!Streaming) break;
		} else {
			bool fullFlag = false;
			send(client, &fullFlag, sizeof(fullFlag), 0);

			int tileCount = frame.howManyTiles;
			send(client, &tileCount, sizeof(tileCount), 0);

			// Batch all tile metadata + data into a single buffer
			size_t totalSize = 0;
			for (auto& tile : frame.tiles)
				totalSize += 5 * sizeof(int) + tile.tile.size();

			std::vector<char> buffer(totalSize);
			size_t offset = 0;

			for (auto& tile : frame.tiles) {
				memcpy(buffer.data() + offset, &tile.y, sizeof(int)); offset += sizeof(int);
				memcpy(buffer.data() + offset, &tile.x, sizeof(int)); offset += sizeof(int);
				memcpy(buffer.data() + offset, &tile.height, sizeof(int)); offset += sizeof(int);
				memcpy(buffer.data() + offset, &tile.width, sizeof(int)); offset += sizeof(int);

				int dataSize = static_cast<int>(tile.tile.size());
				memcpy(buffer.data() + offset, &dataSize, sizeof(int)); offset += sizeof(int);

				memcpy(buffer.data() + offset, tile.tile.data(), tile.tile.size()); offset += tile.tile.size();
			}

			// Send the whole buffer at once
			size_t sent = 0;
			while (sent < buffer.size()) {
				ssize_t n = send(client, buffer.data() + sent, buffer.size() - sent, 0);
				if (n <= 0) { Streaming = false; break; }
				sent += n;
			}
			if (!Streaming) break;
		}

		usleep(33000); // ~30 FPS
	}

	Streaming = false;
	if (client >= 0) {
		close(client);
		client = -1;
	}
}

void Network::killProcessOnPort(int port) {
	std::string cmd = "lsof -t -i :" + std::to_string(port);
	std::array<char, 128> buffer{};
	std::string result;

	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe) {
		std::cerr << "Failed to run lsof\n";
		return;
	}

	while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
		result += buffer.data();

	pclose(pipe);

	if (result.empty()) {
		std::cout << "Nothing is listening on port " << port << "\n";
		return;
	}

	int pid = std::stoi(result);
	std::cout << "Killing PID " << pid << " on port " << port << "\n";

	std::string killCmd = "kill -9 " + std::to_string(pid);
	system(killCmd.c_str());
	usleep(33000);
}
