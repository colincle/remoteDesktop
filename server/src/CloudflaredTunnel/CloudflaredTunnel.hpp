#ifndef CLOUDFLARED_TUNNEL_HPP
#define CLOUDFLARED_TUNNEL_HPP

#include <string>

class CloudflaredTunnel
{
public:
	CloudflaredTunnel(const std::string &cloudflaredPath, int port, const std::string &cloudflaredUrl);
	~CloudflaredTunnel();

	bool start();
	bool isRunning() const;

private:
	std::string m_path;
	std::string m_name;
	int port;
	pid_t m_pid;

	std::string tunnelId;
	std::string credPath;
	std::string certPath;
	std::string credDir;

	// bool runCommand(const std::string &cmd);
	bool runCommandCapture(const std::string &cmd, std::string &output);

	std::string ensureStorageDir();
	bool copyFile(const std::string &src, const std::string &dst);
	std::string generateMachineId(const std::string &storageDir);
	bool loadSavedData(const std::string &storageDir, std::string &machineIdOut, std::string &tunnelNameOut);
	bool createCloudflaredConfigFile(const std::string &storageDir, const std::string &tunnelName);

	bool extractCertPath(const std::string &output);
	bool extractCredPath(const std::string &output);
	std::string getTunnelID(const std::string &credPath);
};

#endif


// #pragma once
// #include <string>

// class CloudflaredTunnel
// {
// private:
//         std::string m_path;
//         int m_port;
//         pid_t m_pid = -1;
//         int m_pipeFd = -1;
//         std::string m_url;

//         std::string parseUrl(const std::string &line);

// public:
//         CloudflaredTunnel(const std::string &cloudflaredPath, int localPort);
//         ~CloudflaredTunnel();

//         bool start();
//         std::string getUrl() const;

//         bool isRunning() const;
// };
