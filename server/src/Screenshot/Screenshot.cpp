#include "Screenshot.hpp"
#include "../../../tools/DebugStatement.hpp"
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QCursor>
#include <QPixmap>

std::vector<uint8_t> Screenshot::captureJPEG(int &width, int &height, int quality)
{
	QScreen* screen = QGuiApplication::primaryScreen();
	if (!screen) {
		DEBUG_OUT << "No primary screen detected!" << std::endl;
		return {};
	}

	QPixmap pix = screen->grabWindow(0);
	if (pix.isNull()) {
		DEBUG_OUT << "Failed to grab screen!" << std::endl;
		return {};
	}

	QImage img = pix.toImage().convertToFormat(QImage::Format_RGBA8888);

	QPainter painter(&img);

	QPixmap cursorPixmap = QCursor().pixmap();
	QPoint cursorPos = QCursor::pos();
	QPoint screenTopLeft = screen->geometry().topLeft();
	QPoint localPos = cursorPos - screenTopLeft;

	if (!cursorPixmap.isNull()) {
		QPixmap scaled = cursorPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		painter.drawPixmap(localPos, scaled);
	} else {
		painter.setBrush(Qt::red);
		painter.setPen(Qt::NoPen);
		painter.drawEllipse(localPos, 8, 8);
	}
	painter.end();

	width = img.width();
	height = img.height();

	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);

	if (!img.save(&buffer, "JPEG", quality)) {
		DEBUG_OUT << "Failed to save image to buffer!" << std::endl;
		return {};
	}

	return std::vector<uint8_t>(ba.begin(), ba.end());
}

bool Screenshot::saveJPEGToFile(const std::string& filename, int quality)
{
	int w, h;
	std::vector<uint8_t> data = captureJPEG(w, h, quality);
	if (data.empty()) return false;

	std::ofstream file(filename, std::ios::binary);
	if (!file) return false;

	file.write(reinterpret_cast<const char*>(data.data()), data.size());
	return file.good();
}
