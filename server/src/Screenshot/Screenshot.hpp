#pragma once
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QImage>
#include <QBuffer>
#include <QByteArray>
#include <fstream>
#include <vector>
#include <string>

class Screenshot
{
public:
    static std::vector<uint8_t> captureJPEG(int &width, int &height, int quality);
    static bool saveJPEGToFile(const std::string& filename, int quality);
};
