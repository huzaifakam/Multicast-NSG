#!/bin/bash

#force the WLAN card to only run in the access point mode

cd
sudo printf "# interfaces(5) file used by ifup(8) and ifdown(8) \n auto lo \n iface lo inet loopback \n auto wlp1s0 \n iface wlp1s0 inet static \n hostapd /etc/hostapd/hostapd.conf \n address 192.168.8.1 \n netmask 255.255.255.0" | tee /etc/network/interfaces
sudo printf "interface=lo,wlp1s0 \n no-dhcp-interface=lo \n dhcp-range=192.168.8.20,192.168.8.254,255.255.255.0,12h" | tee /etc/dnsmasq.conf
