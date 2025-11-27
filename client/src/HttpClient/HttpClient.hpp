#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QByteArray>

class QNetworkReply;

class HttpClient : public QWidget
{
	Q_OBJECT

public:
	explicit HttpClient(const QString &url, QWidget *parent = nullptr);

private slots:
	void onDataAvailable(QNetworkReply* reply); // declare the slot

private:
	void fetchFrame(); // optional, could remove if streaming continuously

	QString m_url;
	QLabel* label;
	QVBoxLayout* layout;
	QNetworkAccessManager* manager;
	QTimer* timer;

	QByteArray m_buffer; // buffer for MJPEG frames
};
