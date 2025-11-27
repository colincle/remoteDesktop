#pragma once

#include <string>
#include <vector>

typedef struct s_token
{
    int line;
    int col;
    std::string token;
}   t_token;

typedef struct s_directive
{
    t_token directive;
    std::vector<t_token> options;
}   t_directive;

typedef struct s_config
{
    int port;
    std::string ip;
}   t_config;
