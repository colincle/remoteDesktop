#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QByteArray>
#include <QImage>

class MjpegClient : public QWidget
{
	Q_OBJECT

public:
	explicit MjpegClient(const QString &url, QWidget *parent = nullptr);

private:
	void processBuffer();

	QLabel* label;
	QVBoxLayout* layout;
	QNetworkAccessManager* manager;
	QString m_url;
	QByteArray m_buffer;
	QImage m_currentImage;
};
