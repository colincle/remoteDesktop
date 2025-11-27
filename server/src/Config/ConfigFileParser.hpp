#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "../../../include/Macros.hpp"
#include "../../../include/DataTypes.hpp"
#include "../../../tools/ErrorWarning.hpp"

class ConfigFileParser
{
private:
    std::filesystem::path filePath;
    t_config config;

public:
    ConfigFileParser();

    //======================================================================================GETTERS
    const t_config& getConfig();

private:
    //======================================================================================PARSING
    std::vector<std::string> loadConfigFile();
    std::vector<t_token> tokenizeLines(std::vector<std::string>& lines);
    std::vector<t_directive> createDirectives(const std::vector<t_token>& tokens);

    //==============================================================================BUILDING CONFIG
    void fillConfig(const std::vector<t_directive>& directives);
    void setListenConf(const t_directive& directive);

    //======================================================================================HELPERS
    void optionsCount(const t_directive& directive, size_t min, size_t max);
    void checkIP(const t_token& token, std::string& ip);
    void checkPort(const t_token& token, int port);
};

std::ostream& operator<<(std::ostream& os, const t_config& cfg);