#include "HttpClient.hpp"
#include "../../tools/DebugStatement.hpp"
#include <QNetworkReply>
#include <QRegularExpression>

HttpClient::HttpClient(const QString &url, QWidget *parent)
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

	// Start the streaming request
	QNetworkReply* reply = manager->get(QNetworkRequest(QUrl(m_url)));
	connect(reply, &QNetworkReply::readyRead, this, [this, reply]() { onDataAvailable(reply); });
	connect(reply, &QNetworkReply::errorOccurred, this, [reply](QNetworkReply::NetworkError) {
		DEBUG_OUT << "Failed to fetch frame: " << reply->errorString().toStdString() << std::endl;
	});
}

void HttpClient::onDataAvailable(QNetworkReply* reply)
{
	m_buffer.append(reply->readAll());

	while (true)
	{
		int frameStart = m_buffer.indexOf("--frame");
		if (frameStart < 0) break;

		int frameEnd = m_buffer.indexOf("--frame", frameStart + 7);
		if (frameEnd < 0) break;

		QByteArray frameData = m_buffer.mid(frameStart, frameEnd - frameStart);

		// Extract Content-Length using QRegularExpression
		QRegularExpression re("Content-Length: (\\d+)");
		QRegularExpressionMatch match = re.match(frameData);
		int len = 0;
		if (match.hasMatch())
			len = match.captured(1).toInt();

		int imgStart = frameData.indexOf("\r\n\r\n");
		if (imgStart < 0) break;
		imgStart += 4;

		QImage img;
		img.loadFromData(frameData.mid(imgStart, len), "JPEG");

		if (!img.isNull())
			label->setPixmap(QPixmap::fromImage(img).scaled(label->size(),
			                                                Qt::KeepAspectRatio,
			                                                Qt::SmoothTransformation));

		m_buffer.remove(0, frameEnd);
	}
}
