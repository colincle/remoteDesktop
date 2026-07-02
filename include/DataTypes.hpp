#pragma once

#include <string>
#include <vector>
#include <QImage>

struct t_token
{
    int line;
    int col;
    std::string token;
};

struct t_directive
{
    t_token directive;
    std::vector<t_token> options;
};

struct t_config
{
    std::string ip;
    int portStart;
    int portEnd;
    int port;
    std::string cloudflaredUrl;
};

struct t_dirtyTile
{
    int y, x, height, width;
    std::vector<uint8_t> tile;
};

struct t_frame
{
    bool fullFrame = false;
    int howManyTiles = 0;
    std::vector<uint8_t> screen;
    std::vector<t_dirtyTile> tiles;
    QImage lastFrameImage;
};
