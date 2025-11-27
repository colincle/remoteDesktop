#include "Network.hpp"
#include "../../../tools/DebugStatement.hpp"
#include "../Screenshot/Screenshot.hpp"
#include "../CloudflaredTunnel/CloudflaredTunnel.hpp"
#include <iostream>

Network::Network(const t_config &c) : cfg(c) {}

void Network::launch()
{
	CloudflaredTunnel tunnel(CLOUDFLARED_PATH, cfg.port);
	if (!tunnel.start())
	{
		error("Failed to retrieve cloudflared tunnel.", "Network::launch");
	}
	// Create socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		error("socket failed: " + std::string(strerror(errno)), "Network");
		exit(1);
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(cfg.port);
	addr.sin_addr.s_addr = inet_addr(cfg.ip.c_str());

	int opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		error("setsockopt failed: " + std::string(strerror(errno)), "Network");

	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
		error("bind failed: " + std::string(strerror(errno)), "Network");

	if (listen(sock, SOMAXCONN) < 0)
		error("listen failed: " + std::string(strerror(errno)), "Network");

	DEBUG_OUT << std::endl << CYAN << "NETWORK" << std::endl
			  << "=======" << RESET << std::endl << "Listening on "
			  << cfg.ip << ":" << cfg.port << std::endl
			  << "Tunnel URL: " << tunnel.getUrl() << std::endl;

	bool lookingForClient = true;
	while (lookingForClient)
	{
		int client = accept(sock, NULL, NULL);
		if (client < 0)
		{
			warning("accept failed: " + std::string(strerror(errno)), "Network");
			continue;
		}
		DEBUG_OUT << "Client connected" << std::endl;

		bool Streaming = true;
		std::vector<uint8_t> screenshot;
		int w, h;

		// Write HTTP header once
		std::string header =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
			"Connection: keep-alive\r\n"
			"\r\n";
		send(client, header.c_str(), header.size(), 0);

		while (Streaming)
		{
			screenshot = Screenshot::captureJPEG(w, h, 90);
			if (screenshot.empty()) continue;

			std::string frameHeader =
				"--frame\r\n"
				"Content-Type: image/jpeg\r\n"
				"Content-Length: " + std::to_string(screenshot.size()) + "\r\n\r\n";

			// Send frame header
			size_t sent = 0;
			while (sent < frameHeader.size()) {
				ssize_t n = send(client, frameHeader.data() + sent, frameHeader.size() - sent, 0);
				if (n <= 0) { Streaming = false; break; }
				sent += n;
			}
			if (!Streaming) break;

			// Send frame data
			sent = 0;
			while (sent < screenshot.size()) {
				ssize_t n = send(client, screenshot.data() + sent, screenshot.size() - sent, 0);
				if (n <= 0) { Streaming = false; break; }
				sent += n;
			}
			if (!Streaming) break;

			// Send end-of-frame \r\n
			const char* frameEnd = "\r\n";
			send(client, frameEnd, 2, 0);

			// small sleep to avoid flooding
			usleep(33000); // ~30 FPS
		}

	}
}
