#include <iostream>
#include <QApplication>
#include "./MjpegClient/MjpegClient.hpp"
#include "../../tools/ErrorWarning.hpp"
#include "../../tools/DebugStatement.hpp"

int main(int argc, char* argv[])
{		
	try
	{
		if (argc != 2)
			error("Usage ./Client <server_url>", "main");
		QString url = QString::fromStdString("http://" + std::string(argv[1]));

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
