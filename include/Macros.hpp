#pragma once

#define DEBUG true

#define APP_DATA_DIR ".local/share/remoteDesktop"
#define CONFIG_FILE_NAME "server.conf"
#define CONFIG_FILE_PATH APP_DATA_DIR "/" CONFIG_FILE_NAME


#ifdef __APPLE__
#define CLOUDFLARED_PATH "../cloudflared/mac/cloudflared"
#else
#define CLOUDFLARED_PATH "../cloudflared/linux/cloudflared"
#endif
