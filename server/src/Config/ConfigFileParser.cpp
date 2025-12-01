#include "ConfigFileParser.hpp"
#include "../../../include/Colors.hpp"

#include <sys/stat.h>
#include <sys/types.h>

ConfigFileParser::ConfigFileParser()
{
	std::vector<std::string> lines = loadConfigFile();
	std::vector<t_token> tokens = tokenizeLines(lines);
	std::vector<t_directive> directives = createDirectives(tokens);
	fillConfig(directives);
	checkConfig();
}

//======================================================================================GETTERS
const t_config& ConfigFileParser::getConfig() { return config; }

//======================================================================================PARSING

std::vector<std::string> ConfigFileParser::loadConfigFile()
{
	const char* home = getenv("HOME");
	if (!home) error("HOME environment variable not set", "Config");

	std::string dir = std::string(home) + "/.local/share/remoteDesktop";
	mkdir(dir.c_str(), 0755);

	// just the filename, not the full path
	std::string filePath = dir + "/server.conf";

	std::ifstream file(filePath);
	if (!file)
	{
		std::ofstream out(filePath);
		if (!out) error("Cannot create config file '" + filePath + "'", "Configuration file");

		out << R"(
					# IP the server listens on. 0.0.0.0 = all interfaces
					#listenIP 0.0.0.0;
					listenIP 0.0.0.0;

					# Range of ports the server can use for connections (obsolete, only one port works)
					#listenPortRange 10340 10340;
					listenPortRange 10340 10340;

					# Public URL or hostname where the server is exposed via tunnel
					#cloudflaredUrl example.com;
					cloudflaredUrl example.com;
				)";
		out.close();

		file.open(filePath);
		if (!file) error("Cannot read newly created config file '" + filePath + "'", "Configuration file");
	}

	std::vector<std::string> lines;
	std::string line;
	char c;

	while (file.get(c))
	{
		if (c == '\0')
			error("Null byte detected.", "Configuration file");

		if (c == '\n')
		{
			lines.push_back(line);
			line.clear();
		}
		else if (c != '\r')
			line.push_back(c);
	}

	if (!line.empty())
		lines.push_back(line);

	return lines;
}

std::vector<t_token> ConfigFileParser::tokenizeLines(std::vector<std::string>& lines)
{
	std::vector<t_token> tokens;

	for (size_t i = 0; i < lines.size(); i++)
	{
		size_t pos = lines[i].find_first_of('#');
		lines[i] = lines[i].substr(0, pos);

		size_t col = 0;
		while (col < lines[i].size())
		{
			while (col < lines[i].size() && (lines[i][col] == ' ' || lines[i][col] == '\t'))
				col++;

			if (col >= lines[i].size())
				break;

			size_t start_col = col;

			if (lines[i][col] == ';')
			{
				t_token tok;
				tok.line = i + 1;
				tok.col = start_col + 1;
				tok.token = ";";
				tokens.push_back(tok);
				col++;
				continue;
			}

			while (col < lines[i].size() && lines[i][col] != ' ' && lines[i][col] != '\t' && lines[i][col] != ';')
				col++;

			t_token tok;
			tok.line = i + 1;
			tok.col = start_col + 1;
			tok.token = lines[i].substr(start_col, col - start_col);
			tokens.push_back(tok);
		}
	}

	return tokens;
}

std::vector<t_directive> ConfigFileParser::createDirectives(const std::vector<t_token>& tokens)
{
	std::vector<t_directive> directives;
	size_t i = 0;

	while (i < tokens.size())
	{
		if (tokens[i].token == ";")
		{
			i++;
			continue;
		}

		t_directive dir;
		dir.directive = tokens[i];
		i++;

		while (i < tokens.size() && tokens[i].token != ";")
		{
			dir.options.push_back(tokens[i]);
			i++;
		}

		if (i < tokens.size() && tokens[i].token == ";")
			i++;

		directives.push_back(dir);
	}

	return directives;
}

//==============================================================================BUILDING CONFIG
void ConfigFileParser::fillConfig(const std::vector<t_directive>& directives)
{
	for (size_t i = 0; i < directives.size(); i++)
	{
		t_directive dir = directives[i];

		if (dir.directive.token == "listenIP")
			setListenIP(dir);
		else if (dir.directive.token == "listenPortRange")
			setListenPortRange(dir);
		else if (dir.directive.token == "cloudflaredUrl")
			setCloudflaredUrl(dir);
		else
			error("Unknown directive "
				+ dir.directive.token,
				filePath.string()
				+ ":" + std::to_string(dir.directive.line)
				+ ":" + std::to_string(dir.directive.col));
	}
}

void ConfigFileParser::setListenIP(const t_directive& directive)
{
	optionsCount(directive, 1, 1);
	std::string ip = directive.options[0].token;
	checkIP(directive.options[0], ip);
	config.ip = ip;
	listenIP = true;
}

void ConfigFileParser::setListenPortRange(const t_directive& directive)
{
	optionsCount(directive, 2, 2);
	int portStart, portEnd;

	std::stringstream ss1(directive.options[0].token);
	if (!(ss1 >> portStart) || !ss1.eof())
		error("Invalid port: " + directive.options[0].token,
			filePath.string() + ":" + std::to_string(directive.options[0].line) + ":" + std::to_string(directive.options[0].col));

	std::stringstream ss2(directive.options[1].token);
	if (!(ss2 >> portEnd) || !ss2.eof())
		error("Invalid port: " + directive.options[1].token,
			filePath.string() + ":" + std::to_string(directive.options[1].line) + ":" + std::to_string(directive.options[1].col));

	checkPort(directive.options[0], portStart);
	checkPort(directive.options[1], portEnd);

	if (portEnd < portStart)
		error("listenPortRange end port must be >= start port",
			filePath.string() + ":" + std::to_string(directive.directive.line) + ":" + std::to_string(directive.directive.col));

	config.portStart = portStart;
	config.portEnd = portEnd;
	listenPortRange = true;
}

void ConfigFileParser::setCloudflaredUrl(const t_directive& directive)
{
	optionsCount(directive, 1, 1);
	config.cloudflaredUrl = directive.options[0].token;
	cloudflaredUrl = true;
}

void ConfigFileParser::checkConfig()
{
	if (!listenIP)
		error("Missing configuration: listenIP", "ConfigFileParser");
	if (!listenPortRange)
		error("Missing configuration: listenPortRange", "ConfigFileParser");
	if (!cloudflaredUrl)
		error("Missing configuration: cloudflaredUrl", "ConfigFileParser");
}

//======================================================================================HELPERS
void ConfigFileParser::optionsCount(const t_directive& directive, size_t min, size_t max)
{
	size_t count = directive.options.size();
	if (count < min || count > max)
	{
		error("Directive '" + directive.directive.token + "' expects "
			+ std::to_string(min) + (min == max ? "" : "-" + std::to_string(max))
			+ " option(s), got " + std::to_string(count),
			filePath.string() + ":" + std::to_string(directive.directive.line) + ":" + std::to_string(directive.directive.col));
	}
}

void ConfigFileParser::checkIP(const t_token& token, std::string& ip)
{
	std::stringstream ss(ip);
	std::string segment;
	int count = 0;

	while (std::getline(ss, segment, '.'))
	{
		if (segment.empty() || segment.size() > 3)
			error("Invalid IP: " + ip,
				filePath.string() + ":" + std::to_string(token.line) + ":" + std::to_string(token.col));

		for (std::string::size_type i = 0; i < segment.size(); ++i)
			if (!std::isdigit(segment[i]))
				error("Invalid IP: " + ip,
					filePath.string() + ":" + std::to_string(token.line) + ":" + std::to_string(token.col));

		int num = std::atoi(segment.c_str());
		if (num < 0 || num > 255)
			error("Invalid IP: " + ip,
				filePath.string() + ":" + std::to_string(token.line) + ":" + std::to_string(token.col));

		count++;
	}

	if (count != 4)
		error("Invalid IP: " + ip,
			filePath.string() + ":" + std::to_string(token.line) + ":" + std::to_string(token.col));
}

void ConfigFileParser::checkPort(const t_token& token, int port)
{
	if (port < 1 || port > 65535)
		error("Invalid port: " + std::to_string(port),
			filePath.string() + ":" + std::to_string(token.line) + ":" + std::to_string(token.col));
}

std::ostream& operator<<(std::ostream& os, const t_config& cfg)
{
	os << YELLOW << "\nCONFIGURATION\n=============" << RESET
	   << "\nIP: " << cfg.ip
	   << "\nPort range: " << cfg.portStart << "-" << cfg.portEnd
	   << "\nCloudflared Tunnel URL: " << cfg.cloudflaredUrl
	   << std::endl << std::endl;
	return os;
}
