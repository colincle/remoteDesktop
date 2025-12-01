#pragma once
#include <QGuiApplication>
#include <QScreen>
#include <QImage>
#include "../../../include/DataTypes.hpp"

class Screenshot
{
public:
    static bool newFrame(int &width, int &height, int quality, t_frame& frame);
};
