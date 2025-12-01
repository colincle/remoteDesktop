#!/bin/bash
set -e

CLOUDFLARED="../cloudflared/mac/cloudflared"

# list tunnels and extract names
names=$($CLOUDFLARED tunnel list | awk 'NR>3 {print $2}')

# cleanup + delete each tunnel
for name in $names; do
	$CLOUDFLARED tunnel cleanup "$name"
	$CLOUDFLARED tunnel delete "$name"
done

# wipe app state
rm -rf ~/.local/share/remoteDesktop