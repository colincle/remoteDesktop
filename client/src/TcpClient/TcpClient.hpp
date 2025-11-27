#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <sys/socket.h>

#include <QImage>
#include <QLabel>
#include <QWidget>
#include <QPixmap>
#include <QVBoxLayout>
#include <QTimer>
#include <QGuiApplication>

class TcpClient : public QWidget
{
	Q_OBJECT

public:
	TcpClient(const char* ip, int port, QWidget* parent = nullptr);
	void run();

private:
	const char* ip;
	int port;
	int sock{-1};
	int frame{0};

	QLabel* label{nullptr};
	QVBoxLayout* layout{nullptr};

	bool recvAll(void* buffer, size_t size);
	QImage receiveFrame();
	void updateFrame(const QImage& img);
};
