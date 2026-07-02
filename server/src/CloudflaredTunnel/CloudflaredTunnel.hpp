#pragma once

#include <string>
#include <sys/types.h> // pid_t

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
