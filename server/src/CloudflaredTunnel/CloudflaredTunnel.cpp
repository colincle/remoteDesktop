#include "CloudflaredTunnel.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <regex>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <sstream>

CloudflaredTunnel::CloudflaredTunnel(const std::string &cloudflaredPath, int localPort)
	: m_path(cloudflaredPath), m_port(localPort) {}

CloudflaredTunnel::~CloudflaredTunnel() {
	if (m_pid > 0) {
		kill(m_pid, SIGTERM);
		waitpid(m_pid, nullptr, 0);
	}
	if (m_pipeFd != -1) {
		close(m_pipeFd);
	}
}

bool CloudflaredTunnel::start() {
	int pipefd[2];
	if (pipe(pipefd) < 0)
		return false;

	m_pid = fork();
	if (m_pid < 0)
		return false;

	if (m_pid == 0) {
		// Child
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);

		std::string cmd = m_path + " tunnel --url http://localhost:" + std::to_string(m_port);
		execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
		_exit(1);
	}

	// Parent
	close(pipefd[1]);
	m_pipeFd = pipefd[0];

	char buffer[2048];
	bool inCreatedBlock = false;

	while (true) {
		ssize_t r = read(m_pipeFd, buffer, sizeof(buffer) - 1);
		if (r <= 0)
			break;

		buffer[r] = '\0';
		std::string chunk(buffer);

		std::stringstream ss(chunk);
		std::string line;

		while (std::getline(ss, line)) {

			if (line.find("Your quick Tunnel has been created!") != std::string::npos) {
				inCreatedBlock = true;
				continue;
			}

			if (inCreatedBlock) {
				std::string url = parseUrl(line);
				if (!url.empty()) {
					m_url = url;
					return true;
				}
			}
		}
	}

	return false;
}

std::string CloudflaredTunnel::parseUrl(const std::string &line) {
	static const std::regex urlRegex(R"(https://[a-zA-Z0-9\-]+\.trycloudflare\.com)");
	std::smatch match;
	if (std::regex_search(line, match, urlRegex))
		return match.str();
	return "";
}

std::string CloudflaredTunnel::getUrl() const {
	return m_url;
}
