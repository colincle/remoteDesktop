#include "MjpegClient.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDataStream>
#include <QPainter>
#include <iostream>

MjpegClient::MjpegClient(const QString &url, QWidget *parent)
	: QWidget(parent), m_url(url)
{
	setWindowTitle("Screen Viewer");
	resize(800, 600);

	label = new QLabel(this);
	label->setAlignment(Qt::AlignCenter);

	layout = new QVBoxLayout(this);
	layout->addWidget(label);
	setLayout(layout);

	manager = new QNetworkAccessManager(this);
	QNetworkReply* reply = manager->get(QNetworkRequest(QUrl(m_url)));

	connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
		QByteArray chunk = reply->readAll();
		std::cout << "Received " << chunk.size() << " bytes" << std::endl;
		std::cout << "First bytes: " << chunk.left(50).toHex().toStdString() << std::endl;

		m_buffer.append(chunk);      // <- append first
		processBuffer();             // <- then process
	});


	connect(reply, &QNetworkReply::errorOccurred, this, [reply](QNetworkReply::NetworkError) {
		std::cerr << "Network error occurred: " << reply->errorString().toStdString() << std::endl;
	});
}

void MjpegClient::processBuffer()
{
	QDataStream stream(&m_buffer, QIODevice::ReadOnly);
	stream.setByteOrder(QDataStream::LittleEndian);

	while (true) {
		if (stream.atEnd()) break;
		stream.device()->seek(0);

		if (m_buffer.size() < static_cast<qsizetype>(sizeof(bool))) break;
		bool fullFrame;
		stream >> fullFrame;

		if (fullFrame) {
			if (m_buffer.size() < static_cast<qsizetype>(sizeof(bool) + sizeof(int))) break;
			int dataSize;
			stream >> dataSize;

			if (m_buffer.size() < static_cast<qsizetype>(sizeof(bool) + sizeof(int) + dataSize)) break;

			QByteArray imgData = m_buffer.mid(sizeof(bool) + sizeof(int), dataSize);
			QImage img;
			if (img.loadFromData(imgData, "JPEG")) {
				label->setPixmap(QPixmap::fromImage(img).scaled(label->size(),
																Qt::KeepAspectRatio,
																Qt::SmoothTransformation));
				m_currentImage = img;
			}

			m_buffer.remove(0, sizeof(bool) + sizeof(int) + dataSize);
		} else {
			if (m_buffer.size() < static_cast<qsizetype>(sizeof(bool) + sizeof(int))) break;
			int tileCount;
			stream >> tileCount;

			int headerSize = sizeof(bool) + sizeof(int);
			bool enoughData = true;

			if (m_currentImage.isNull()) {
				int maxW = 0, maxH = 0;
				for (int i = 0; i < tileCount; ++i) {
					if (m_buffer.size() < static_cast<qsizetype>(headerSize + 5 * sizeof(int))) { enoughData = false; break; }
					int y, x, h, w, dataSize;
					memcpy(&y, m_buffer.data() + headerSize, sizeof(int));
					memcpy(&x, m_buffer.data() + headerSize + sizeof(int), sizeof(int));
					memcpy(&h, m_buffer.data() + headerSize + 2 * sizeof(int), sizeof(int));
					memcpy(&w, m_buffer.data() + headerSize + 3 * sizeof(int), sizeof(int));
					memcpy(&dataSize, m_buffer.data() + headerSize + 4 * sizeof(int), sizeof(int));
					maxW = std::max(maxW, x + w);
					maxH = std::max(maxH, y + h);
					headerSize += 5 * sizeof(int) + dataSize;
				}
				m_currentImage = QImage(maxW, maxH, QImage::Format_RGB32);
				m_currentImage.fill(Qt::black);
				headerSize = sizeof(bool) + sizeof(int);
			}

			for (int i = 0; i < tileCount; ++i) {
				if (m_buffer.size() < static_cast<qsizetype>(headerSize + 5 * sizeof(int))) { enoughData = false; break; }

				int y, x, h, w, dataSize;
				memcpy(&y, m_buffer.data() + headerSize, sizeof(int));
				memcpy(&x, m_buffer.data() + headerSize + sizeof(int), sizeof(int));
				memcpy(&h, m_buffer.data() + headerSize + 2 * sizeof(int), sizeof(int));
				memcpy(&w, m_buffer.data() + headerSize + 3 * sizeof(int), sizeof(int));
				memcpy(&dataSize, m_buffer.data() + headerSize + 4 * sizeof(int), sizeof(int));

				if (m_buffer.size() < static_cast<qsizetype>(headerSize + 5 * sizeof(int) + dataSize)) { enoughData = false; break; }

				QByteArray tileData = m_buffer.mid(headerSize + 5 * sizeof(int), dataSize);
				QImage tileImg;
				if (!tileData.isEmpty() && tileImg.loadFromData(tileData, "JPEG")) {
					QPainter p(&m_currentImage);
					p.drawImage(x, y, tileImg);
				}

				headerSize += 5 * sizeof(int) + dataSize;
			}

			if (!enoughData) break;
			label->setPixmap(QPixmap::fromImage(m_currentImage).scaled(label->size(),
																	   Qt::KeepAspectRatio,
																	   Qt::SmoothTransformation));
			m_buffer.remove(0, headerSize);
		}
	}
}
