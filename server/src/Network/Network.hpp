#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdlib>

#include "../../../tools/ErrorWarning.hpp"
#include "../../../tools/DebugStatement.hpp"
#include "../../../include/DataTypes.hpp"
#include "../../../include/Colors.hpp"

class Network
{
private:
	t_config cfg;

public:
	Network(const t_config &c);
	void launch();
};
