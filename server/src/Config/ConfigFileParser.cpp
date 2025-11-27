#include "ConfigFileParser.hpp"
#include "../../../include/Colors.hpp"

ConfigFileParser::ConfigFileParser()
{
    std::vector<std::string> lines = loadConfigFile();
    std::vector<t_token> tokens = tokenizeLines(lines);
    std::vector<t_directive> directives = createDirectives(tokens);
    fillConfig(directives);
}

//======================================================================================GETTERS
const t_config& ConfigFileParser::getConfig() { return config; }

//======================================================================================PARSING
std::vector<std::string> ConfigFileParser::loadConfigFile()
{
    filePath = std::filesystem::absolute(CONFIG_FILE_PATH).lexically_normal();

    // Remove a leading "./" from the path itself
    if (!filePath.empty() && filePath.begin()->string() == ".")
        filePath = filePath.relative_path(); // drops the first "." component

    std::ifstream file(filePath);

    if (!file)
        error("file '" + filePath.string() + "' not found. Server will not start.", "Configuration file");

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
        if (dir.directive.token == "listen")
            setListenConf(dir);
        // insert other functions here
        else
            error("Unknown directive "
                + dir.directive.token,
                filePath.string()
                + ":" + std::to_string(dir.directive.line)
                + ":" + std::to_string(dir.directive.col));
    }
}

void ConfigFileParser::setListenConf(const t_directive& directive)
{
    optionsCount(directive, 1, 1);
    std::string ipPort = directive.options[0].token;
    size_t colon = ipPort.find(':');

    if (colon == std::string::npos)
        ipPort = "0.0.0.0:" + ipPort;

    colon = ipPort.find(':');
    std::string ip = ipPort.substr(0, colon);
    std::string portStr = ipPort.substr(colon + 1);
    int port;
    std::stringstream ss(portStr);

    if (!(ss >> port) || !ss.eof())
        error("Invalid port: " + portStr,
            filePath.string() + ":" + std::to_string(directive.options[0].line) + ":" + std::to_string(directive.options[0].col));

    checkIP(directive.options[0], ip);
    checkPort(directive.options[0], port);
    config.ip = ip;
    config.port = port;
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
    os << std::endl <<
    YELLOW << "CONFIGURATION" << std::endl << "=============" << RESET << std::endl <<
    "IP: " << cfg.ip << std::endl << "Port: " << cfg.port << std::endl;
    return os;
}
