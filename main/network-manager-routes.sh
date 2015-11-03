#!/bin/bash
# Inspired by http://lamarque-lvs.blogspot.com/2011/11/networkmanager-per-device-routing.html

# This script will live in /etc/NetworkManager/dispatcher.d

export PATH="/bin:/sbin:/usr/bin:/usr/sbin"

IFACE_NOTIFY_PID=/var/run/multilink-iface-notify.pid

get_table_num() {
    id=$(cat "/sys/class/net/$1/ifindex")
    echo $((id+1000))
}

case $2 in
    up|dhcp4-change)
        IP_PREFIX=$(echo $IP4_ADDRESS_0 | cut -d ' ' -f 1 | cut -d / -f 2)

        ip route add $DHCP4_NETWORK_NUMBER/$IP_PREFIX dev $DEVICE_IP_IFACE src $DHCP4_IP_ADDRESS table $DEVICE_IP_IFACE
        ip route add $DHCP4_NETWORK_NUMBER/$IP_PREFIX dev $DEVICE_IP_IFACE src $DHCP4_IP_ADDRESS
        ip route add default via $(echo $DHCP4_ROUTERS | cut -d ' ' -f 1) table $DEVICE_IP_IFACE
        TABLE_NUM=$(get_table_num $DEVICE_IP_IFACE)
        ip rule add from $DHCP4_IP_ADDRESS table $TABLE_NUM

        [[ -e "$IFACE_NOTIFY_PID" ]] && kill -USR1 "$IFACE_NOTIFY_PID"

        true
    ;;

    down)
        ip rule del table $(get_table_num "$1")
    ;;

    *)
        exit 0
    ;;
esac
