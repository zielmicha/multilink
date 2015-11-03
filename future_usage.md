## Client

The best way to run the client is to use [netd's](https://github.com/networkosnet/netd) `multilink-client` command. However, if you don't want to use netd, you can also run multilink client standalone (optionally with Network Manager integration).

```
client your-ip 443;
identity your-login;
psk your-key;

# Proxy connections from localhost:7000 to the server
# proxy localhost:7000
# Or run VPN:
vpn-client tun0;

# Either: configure routing tables and connection manually
# bind 192.168.1.30 my-wired;
# bind 10.10.0.55 orange-mobile-internet;

# Or use Network Manager integration:
network-manager-integration;
```

## Server

```
server 0.0.0.0 443;
# Lookup PSK in the file
identities-file identities.list;
# Or request it from this socket (writes identity followed by \0) and reads
# PSK until connection is closed
identity-get /var/run/ml-identity.sock;
# Proxy connection from clients to localhost:7000
# target localhost:7000
# Or run VPN server:
vpn-server tun0;
```
