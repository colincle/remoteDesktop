#include <iostream>
#include <QApplication>
#include "./HttpClient/HttpClient.hpp"
#include "../../tools/ErrorWarning.hpp"
#include "../../tools/DebugStatement.hpp"

void getUrl(int argc, char* argv[], QString &url)
{
	if (argc != 2)
		error("Usage: " + std::string(argv[0]) + " <URL>\n", argv[0]);

	url = QString::fromUtf8(argv[1]);
}

int main(int argc, char* argv[])
{
	try
	{
		QString url;
		getUrl(argc, argv, url);

		QApplication app(argc, argv);
		HttpClient client(url);
		client.show();

		return app.exec();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
