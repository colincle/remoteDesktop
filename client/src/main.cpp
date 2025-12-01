#include <iostream>
#include <QApplication>
#include "./MjpegClient/MjpegClient.hpp"
#include "../../tools/ErrorWarning.hpp"
#include "../../tools/DebugStatement.hpp"

// void getUrl(int argc, char* argv[], QString &url)
// {
// 	if (argc != 2)
// 		error("Usage: " + std::string(argv[0]) + " <URL>\n", argv[0]);

// 	url = QString::fromUtf8(argv[1]);

// 	for (int i = 0; i < url.size(); i++)
// 		url[i] = QChar(url[i].unicode() - 1);

// 	url = "https://" + url + ".trycloudflare.com";
// }


int main(int argc, char* argv[])
{		
	try
	{
		if (argc != 2)
			error("Usage ./Client <server_url>", "main");
		QString url = QString::fromStdString("http://" + std::string(argv[1]));
		// getUrl(argc, argv, url);

		QApplication app(argc, argv);
		MjpegClient client(url);
		client.show();

		return app.exec();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
