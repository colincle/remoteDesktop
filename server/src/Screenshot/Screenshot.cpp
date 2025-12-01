#include "Screenshot.hpp"
#include "../../../tools/DebugStatement.hpp"
#include <QPainter>
#include <QCursor>
#include <QPixmap>
#include <QBuffer>
#include <vector>
#include <cstdint>

bool Screenshot::newFrame(int &width, int &height, int quality, t_frame& frame)
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) {
        DEBUG_OUT << "No primary screen detected!" << std::endl;
        return false;
    }

    QPixmap pix = screen->grabWindow(0);
    if (pix.isNull()) {
        DEBUG_OUT << "Failed to grab screen!" << std::endl;
        return false;
    }

    QImage img = pix.toImage().convertToFormat(QImage::Format_RGBA8888);

    // Draw cursor
    QPainter painter(&img);
    QPixmap cursorPixmap = QCursor().pixmap();
    QPoint cursorPos = QCursor::pos() - screen->geometry().topLeft();

    if (!cursorPixmap.isNull()) {
        QPixmap scaled = cursorPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap(cursorPos, scaled);
    } else {
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(cursorPos, 8, 8);
    }
    painter.end();

    width = img.width();
    height = img.height();

    // First frame? send everything
    if (frame.screen.empty()) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        if (!img.save(&buffer, "JPEG", quality)) {
            DEBUG_OUT << "Failed to compress first frame!" << std::endl;
            return false;
        }

        frame.fullFrame = true;
        frame.howManyTiles = 0;
        frame.screen.assign(ba.begin(), ba.end());
        frame.tiles.clear();
        frame.lastFrameImage = img;
        return true;
    }

    // Compare tiles
    const int tileRows = 20;
    const int tileCols = 20;
    int tileWidth = width / tileCols;
    int tileHeight = height / tileRows;
    std::vector<t_dirtyTile> dirtyTiles;

    for (int ty = 0; ty < tileRows; ++ty) {
        for (int tx = 0; tx < tileCols; ++tx) {
            bool dirty = false;
            for (int y = 0; y < tileHeight && !dirty; ++y) {
                for (int x = 0; x < tileWidth && !dirty; ++x) {
                    int px = tx * tileWidth + x;
                    int py = ty * tileHeight + y;
                    if (frame.lastFrameImage.pixel(px, py) != img.pixel(px, py)) {
                        dirty = true;
                    }
                }
            }

            if (dirty) {
                int startX = tx * tileWidth;
                int startY = ty * tileHeight;
                int w = tileWidth;
                int h = tileHeight;
                QImage tileImg = img.copy(startX, startY, w, h);

                QByteArray tileBA;
                QBuffer buffer(&tileBA);
                buffer.open(QIODevice::WriteOnly);
                if (!tileImg.save(&buffer, "JPEG", quality)) continue;

                t_dirtyTile tile = { startY, startX, h, w, std::vector<uint8_t>(tileBA.begin(), tileBA.end()) };
                dirtyTiles.push_back(tile);
            }
        }
    }

    if (dirtyTiles.empty()) {
        // Nothing changed
        frame.fullFrame = false;
        frame.howManyTiles = 0;
        frame.tiles.clear();
        return false;
    }

    frame.tiles = dirtyTiles;
    frame.fullFrame = false;
    frame.howManyTiles = static_cast<int>(dirtyTiles.size());
    frame.lastFrameImage = img;
    return true;
}
