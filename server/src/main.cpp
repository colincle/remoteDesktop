#include "../../tools/DebugStatement.hpp"
#include "./Config/ConfigFileParser.hpp"
#include "./Network/Network.hpp"
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    t_config config;
    try
    {
        ConfigFileParser parser;
        config = parser.getConfig();
        DEBUG_OUT << config;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        return 1;
    }
    Network server(config);
    try
    {
        server.runServer();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        return 1;
    }
}