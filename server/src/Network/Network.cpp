#include "Network.hpp"
#include "../../../tools/DebugStatement.hpp"
#include "../Screenshot/Screenshot.hpp"

Network::Network(const t_config &c) : cfg(c) {}

void Network::launch()
{
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
		<< "=======" << RESET << std::endl << "Listening on " << std::endl
		<< cfg.ip << ":" << cfg.port << std::endl;

//TESTS=========================================================================

	// screenshot.saveJPEGToFile("coolio.JPEG", 90);

//TESTS=========================================================================
	bool lookingForClient = true;
	while (lookingForClient)
	{
		int client = accept(sock, NULL, NULL);
		if (client < 0)
		{
			warning("accept failed: " + std::string(strerror(errno)), "Network");
			continue;
		}
		DEBUG_OUT << "Cool client connected" << std::endl;
		
		bool Streaming = true;
		std::vector<uint8_t> screenshot;
		int w, h;
		while (Streaming)
		{
			screenshot = Screenshot::captureJPEG(w, h, 90);
			if (!screenshot.empty())
			{

				uint32_t size = htonl(screenshot.size());
				if (send(client, &size, sizeof(size), 0) < 0)
				{
					warning("Failed to send size", "Network");
					break;
				}

				size_t totalSent = 0;
				while (totalSent < screenshot.size())
				{
					ssize_t sent = send(client, screenshot.data() + totalSent, screenshot.size() - totalSent, 0);
					if (sent <= 0)
					{
						warning("Failed to send screenshot", "Network");
						Streaming = false;
						break;
					}
					totalSent += sent;
				}
			}
			else
			{
				DEBUG_OUT << "Screenshot capture failed" << std::endl;
			}
		}
	}
}
