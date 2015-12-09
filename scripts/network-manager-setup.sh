#!/bin/bash -e
src="$(dirname "$0")/network-manager-routes.sh"
dst="/etc/NetworkManager/dispatcher.d/multilink-setup-routes.sh"
if ! cmp --silent "$src" "$dst"; then
    echo "Copying script and restarting NetworkManager..."
    cp $src $dst
    chmod +x $dst
    if command -v systemctl &>/dev/null; then
        systemctl restart network-manager
    else
        service network-manager restart
    fi
    echo "Ok."
else
    echo "NetworkManager script already set up."
fi
