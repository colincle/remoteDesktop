#pragma once

#define DEBUG true

#define CONFIG_FILE_PATH "./server.conf"

#ifdef __APPLE__
#define CLOUDFLARED_PATH "../cloudflared/mac/cloudflared"
#else
#define CLOUDFLARED_PATH "../cloudflared/linux/cloudflared"
#endif
