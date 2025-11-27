#include "TcpClient.hpp"
#include "../../tools/ErrorWarning.hpp"
#include "../../tools/DebugStatement.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <QTimer>

TcpClient::TcpClient(const char* ip, int port, QWidget* parent)
	: QWidget(parent), ip(ip), port(port)
{
	setWindowTitle("Screen Viewer");
	resize(800, 600);

	label = new QLabel(this);
	label->setAlignment(Qt::AlignCenter);

	layout = new QVBoxLayout(this);
	layout->addWidget(label);
	setLayout(layout);
}

bool TcpClient::recvAll(void* buffer, size_t size)
{
	size_t received = 0;
	while (received < size)
	{
		ssize_t ret = ::recv(sock, (char*)buffer + received, size - received, 0);
		if (ret <= 0) return false;
		received += ret;
	}
	return true;
}

QImage TcpClient::receiveFrame()
{
	uint32_t sizeNet;
	if (!recvAll(&sizeNet, sizeof(sizeNet))) return {};

	uint32_t size = ntohl(sizeNet);
	if (size == 0) return {};

	std::vector<uint8_t> buffer(size);
	if (!recvAll(buffer.data(), size)) return {};

	QImage img;
	img.loadFromData(buffer.data(), size, "JPEG");
	return img;
}

void TcpClient::updateFrame(const QImage& img)
{
	if (!img.isNull())
	{
		label->setPixmap(QPixmap::fromImage(img).scaled(label->size(), Qt::KeepAspectRatio));
	}
}

void TcpClient::run()
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        error("socket failed", "TcpClient");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
        error("inet_pton failed", "TcpClient");

    if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
        error("connect failed", "TcpClient");

    DEBUG_OUT << "Connected to server " << ip << ":" << port << std::endl;

    // Qt window setup
    QWidget* window = new QWidget();
    window->setWindowTitle("Remote Screen");
    QLabel* label = new QLabel(window);
    QVBoxLayout* layout = new QVBoxLayout(window);
    layout->addWidget(label);
    window->setLayout(layout);
    window->resize(800, 600);
    window->show();

    // Timer for polling frames
    QTimer* timer = new QTimer(this);
    int frame = 0;

    connect(timer, &QTimer::timeout, this, [this, timer, label, &frame]() {
        uint32_t sizeNet;
        ssize_t ret = recv(sock, &sizeNet, sizeof(sizeNet), MSG_WAITALL);
        if (ret <= 0) {
            DEBUG_OUT << "Disconnected from server" << std::endl;
            timer->stop();
            ::close(sock);
            sock = -1;
            return;
        }

        uint32_t size = ntohl(sizeNet);
        if (size == 0) return;

        std::vector<uint8_t> buffer(size);
        ret = recv(sock, buffer.data(), size, MSG_WAITALL);
        if (ret <= 0) {
            DEBUG_OUT << "Disconnected from server" << std::endl;
            timer->stop();
            ::close(sock);
            sock = -1;
            return;
        }

        QImage img = QImage::fromData(buffer.data(), buffer.size(), "JPEG");
        if (!img.isNull()) {
            label->setPixmap(QPixmap::fromImage(img).scaled(label->size(),
                                                            Qt::KeepAspectRatio,
                                                            Qt::SmoothTransformation));
        }
        frame++;
    });

    timer->start(30); // ~33 FPS
}
