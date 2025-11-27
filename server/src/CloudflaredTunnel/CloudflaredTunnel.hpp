#pragma once
#include <string>

class CloudflaredTunnel {
public:
	CloudflaredTunnel(const std::string &cloudflaredPath, int localPort);
	~CloudflaredTunnel();

	bool start();
	std::string getUrl() const;

private:
	std::string parseUrl(const std::string &line);

private:
	std::string m_path;
	int m_port;
	std::string m_url;
	pid_t m_pid = -1;
	int m_pipeFd = -1;
};
