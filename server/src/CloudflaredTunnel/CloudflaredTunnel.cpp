#include "CloudflaredTunnel.hpp"
#include "../../../tools/ErrorWarning.hpp"
#include "../../../include/Macros.hpp"
#include "../../../include/Colors.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <regex>
#include <cstdlib>
#include <cstring>
#include <ctime>

CloudflaredTunnel::CloudflaredTunnel(const std::string &cloudflaredPath, int port, const std::string &cloudflaredUrl)
	: m_path(cloudflaredPath), m_name(cloudflaredUrl), port(port), m_pid(-1) {}

CloudflaredTunnel::~CloudflaredTunnel() {
	if (m_pid > 0) {
		kill(m_pid, SIGTERM);
		waitpid(m_pid, nullptr, 0);
	}
}

// Run command and capture combined stdout+stderr into output.
// Prints only the special login lines (keeps previous behavior).
bool CloudflaredTunnel::runCommandCapture(const std::string &cmd, std::string &output)
{
	int pipefd[2];
	if (pipe(pipefd) < 0) return false;

	pid_t pid = fork();
	if (pid < 0) return false;

	if (pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);
		execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
		_exit(1);
	}

	close(pipefd[1]);
	char buffer[1024];
	output.clear();
	ssize_t n;
	while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
		buffer[n] = '\0';
		std::string chunk(buffer);
		output += chunk;

		// keep the minimal visible output the same as before
		if (chunk.find("A browser window should have opened at the following URL:") != std::string::npos ||
		    chunk.find("https://dash.cloudflare.com/argotunnel") != std::string::npos ||
		    chunk.find("If the browser failed to open") != std::string::npos ||
		    chunk.find("Waiting for login") != std::string::npos) 
		{
		std::cout << CYAN << "\n========================== Cloudflared login ===========================\n\n" << RESET;
		std::cout << std::endl << chunk << GREEN << "\nPlease log in and select the domain you wish to use.\n" << RESET << std::endl;
		}
	}
	close(pipefd[0]);

	int status = 0;
	waitpid(pid, &status, 0);
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

// Helper: ensure the app storage dir exists and return it (absolute)
std::string CloudflaredTunnel::ensureStorageDir()
{
	const char *home = getenv("HOME");
	if (!home) return std::string();
	std::string dir = std::string(home) + "/" + APP_DATA_DIR;

	// mkdir -p semantics
	size_t pos = 0;
	while ((pos = dir.find('/', pos + 1)) != std::string::npos) {
		std::string sub = dir.substr(0, pos);
		mkdir(sub.c_str(), 0700);
	}
	mkdir(dir.c_str(), 0700);
	return dir;
}

// Helper: copy file path src -> dst (overwrites)
bool CloudflaredTunnel::copyFile(const std::string &src, const std::string &dst)
{
	std::ifstream in(src, std::ios::binary);
	if (!in.is_open()) return false;
	std::ofstream out(dst, std::ios::binary | std::ios::trunc);
	if (!out.is_open()) return false;
	char buf[4096];
	while (in.read(buf, sizeof(buf)))
		out.write(buf, in.gcount());
	if (in.gcount() > 0) out.write(buf, in.gcount());
	return true;
}

// Generate a reasonably-unique machine id and persist it
std::string CloudflaredTunnel::generateMachineId(const std::string &storageDir)
{
	std::string idPath = storageDir + "/machine_id";
	std::ifstream existing(idPath);
	if (existing.is_open()) {
		std::string id;
		std::getline(existing, id);
		if (!id.empty()) return id;
	}

	char hn[256];
	if (gethostname(hn, sizeof(hn)) != 0) strncpy(hn, "host", sizeof(hn));

	std::string hostname = hn;

	// Remove ".local" and anything after it
	std::size_t pos = hostname.find(".local");
	if (pos != std::string::npos) {
		hostname = hostname.substr(0, pos);
	}

	std::stringstream ss;
	std::srand((unsigned)std::time(nullptr) ^ getpid());
	ss << hostname << "-" << std::hex << (std::rand() ^ (int)time(nullptr));

	std::string id = ss.str();

	std::ofstream out(idPath, std::ios::trunc);
	if (out.is_open()) out << id << std::endl;

	return id;
}

// Attempt to load previously saved metadata; returns true if everything needed was loaded.
bool CloudflaredTunnel::loadSavedData(const std::string &storageDir, std::string &machineIdOut, std::string &tunnelNameOut)
{
	std::string idPath = storageDir + "/machine_id";
	std::ifstream f(idPath);
	if (!f.is_open()) return false;
	std::getline(f, machineIdOut);
	if (machineIdOut.empty()) return false;

	// expect saved credential JSON and cert and config
	std::string cred = storageDir + "/cred.json";
	std::string cert = storageDir + "/cert.pem";
	std::string cfg  = storageDir + "/config.yml";
	if (!std::ifstream(cred).good() || !std::ifstream(cert).good() || !std::ifstream(cfg).good())
		return false;

	// set members from saved files
	credPath = cred;
	certPath = cert;
	credDir = storageDir;
	tunnelId = getTunnelID(credPath);
	tunnelNameOut = machineIdOut;
	return true;
}

bool CloudflaredTunnel::createCloudflaredConfigFile(const std::string &storageDir, const std::string &tunnelName)
{
	std::string filename = storageDir + "/config.yml";
	std::ofstream file(filename, std::ios::trunc);
	if (!file.is_open()) return false;

	file << "tunnel: " << tunnelId << "\n";
	file << "credentials-file: " << credPath << "\n\n";
	file << "ingress:\n";
	file << "  - hostname: " << tunnelName << "." << m_name << "\n";
	file << "    service: http://localhost:" << port << "\n";
	file << "  - service: http_status:404\n";

	return true;
}

bool CloudflaredTunnel::extractCertPath(const std::string &output)
{
	std::regex re_saved("saved to:\\s*(.*\\.pem)");
	std::regex re_existing("existing certificate at\\s*(.*\\.pem)");
	std::smatch match;

	if (std::regex_search(output, match, re_saved) && match.size() > 1) {
		certPath = match.str(1);
		return true;
	}

	if (std::regex_search(output, match, re_existing) && match.size() > 1) {
		certPath = match.str(1);
		return true;
	}

	return false;
}

bool CloudflaredTunnel::extractCredPath(const std::string &output) {
	std::regex re("Tunnel credentials written to\\s*(.*\\.json)");
	std::smatch match;
	if (std::regex_search(output, match, re) && match.size() > 1) {
		credPath = match.str(1);

		size_t slash = credPath.find_last_of('/');
		if (slash != std::string::npos)
			credDir = credPath.substr(0, slash);
		else
			credDir = ".";

		return true;
	}
	return false;
}

std::string CloudflaredTunnel::getTunnelID(const std::string &credPath) {
	std::ifstream f(credPath);
	if (!f.is_open()) return "";

	std::string content((std::istreambuf_iterator<char>(f)),
	                    std::istreambuf_iterator<char>());

	std::regex re("\"TunnelID\"\\s*:\\s*\"([^\"]+)\"");
	std::smatch match;

	if (std::regex_search(content, match, re) && match.size() > 1)
		return match[1];

	return "";
}

bool CloudflaredTunnel::start()
{
	std::string output;
	std::string storageDir = ensureStorageDir();
	if (storageDir.empty()) return false;

	std::string machineId;
	std::string tunnelName;
	bool newTunnel = false;

	if (!loadSavedData(storageDir, machineId, tunnelName)) {
		newTunnel = true;

		if (!runCommandCapture(m_path + " login", output)) return false;
		if (!extractCertPath(output)) return false;

		std::string certDst = storageDir + "/cert.pem";
		if (!copyFile(certPath, certDst)) return false;
		certPath = certDst;

		machineId = generateMachineId(storageDir);
		tunnelName = machineId;

		if (!runCommandCapture(m_path + " tunnel create " + tunnelName, output)) return false;
		if (!extractCredPath(output)) return false;

		std::string credDst = storageDir + "/cred.json";
		if (!copyFile(credPath, credDst)) return false;
		credPath = credDst;
		credDir = storageDir;
		tunnelId = getTunnelID(credPath);
		if (!createCloudflaredConfigFile(storageDir, tunnelName)) return false;
	}

	std::string configPath = storageDir + "/config.yml";
	std::string targetHostname = tunnelName + "." + m_name;

	// Did the effective hostname change since the last run? This covers editing
	// cloudflaredUrl in server.conf. Read the old config before regenerating it.
	bool hostnameChanged = true;
	{
		std::ifstream existing(configPath);
		if (existing) {
			std::string old((std::istreambuf_iterator<char>(existing)), std::istreambuf_iterator<char>());
			hostnameChanged = old.find("hostname: " + targetHostname) == std::string::npos;
		}
	}

	// Keep config.yml in sync with the current domain on every run, so a domain
	// change actually takes effect instead of being silently ignored.
	if (!newTunnel)
		createCloudflaredConfigFile(storageDir, tunnelName);

	m_pid = fork();
	if (m_pid == 0) {
		int devnull = open("/dev/null", O_RDWR);
		dup2(devnull, STDOUT_FILENO);
		dup2(devnull, STDERR_FILENO);
		close(devnull);

		execl(
			m_path.c_str(),
			m_path.c_str(),
			"tunnel",
			"--config",
			configPath.c_str(),
			"run",
			tunnelName.c_str(),
			nullptr
		);
		_exit(1);
	}

	if (newTunnel || hostnameChanged)
	{
		std::string dnsUrl = "https://dash.cloudflare.com/?to=/:account/:zone/dns/records";
#if defined(__APPLE__)
		std::string openCmd = "open \"" + dnsUrl + "\"";
#else
		std::string openCmd = "xdg-open \"" + dnsUrl + "\"";
#endif
		system(openCmd.c_str());

		std::cout << MAGENTA << "\n========================== Cloudflared setup ===========================\n\n" << RESET;

		std::cout << "1. A browser window should have opened. If it didn't, open this URL yourself:\n";
		std::cout << dnsUrl << "\n\n";

		std::cout << "2. In the DNS dashboard, click 'Add Record'.\n";
		std::cout << "   Type: " << GREEN << "CNAME" << RESET <<"\n";
		std::cout << "   Name: " << GREEN <<  tunnelName << RESET << "\n";
		std::cout << "   Target: " << GREEN << tunnelId << RESET << ".cfargotunnel.com\n";
		std::cout << "   Proxy status: " << RED << "Do not uncheck.\n\n" << RESET;

		std::cout << "3. Save the record. DNS will propagate automatically.\n\n";

		std::cout << "When done, you can connect using:\n";
		std::cout  << GREEN << tunnelName << "." << m_name << RESET << "\n";

		std::cout << MAGENTA <<"\n========================================================================\n" << RESET;
	}
	else
	{
		std::cout << MAGENTA << "\n==================== Cloudflared setup successful ======================\n\n" << RESET;
		std::cout << "Use the address "  << GREEN << tunnelName << "." << m_name << RESET << " to connect to this server." << std::endl;
		std::cout << MAGENTA << "\n========================================================================\n" << RESET;
	}
	return true;
}

bool CloudflaredTunnel::isRunning() const {
	return m_pid > 0;
}
