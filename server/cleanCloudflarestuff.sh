#!/bin/bash
set -e

# Pick the binary for this OS, matching CLOUDFLARED_PATH in include/Macros.hpp.
if [[ "$(uname)" == "Darwin" ]]; then
	CLOUDFLARED="../cloudflared/mac/cloudflared"
else
	CLOUDFLARED="../cloudflared/linux/cloudflared"
fi

# list tunnels and extract names
names=$($CLOUDFLARED tunnel list | awk 'NR>3 {print $2}')

# cleanup + delete each tunnel
for name in $names; do
	$CLOUDFLARED tunnel cleanup "$name"
	$CLOUDFLARED tunnel delete "$name"
done

# wipe app state
rm -rf ~/.local/share/remoteDesktop