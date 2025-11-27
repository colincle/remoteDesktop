#include "./Config/ConfigFileParser.hpp"
#include "../../tools/DebugStatement.hpp"
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
        server.launch();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        return 1;
    }
}