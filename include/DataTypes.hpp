#pragma once

#include <string>
#include <vector>
#include <QImage>

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
	std::string ip;
	int portStart;
	int portEnd;
	int port;
	std::string cloudflaredUrl;
}   t_config;

typedef struct s_dirtyTile
{
    int y, x, height, width;
    std::vector<uint8_t> tile;
} t_dirtyTile;


typedef struct s_frame
{
    bool fullFrame = false;
    int howManyTiles = 0;
    std::vector<uint8_t> screen;
    std::vector<t_dirtyTile> tiles;
    QImage lastFrameImage;
} t_frame;
