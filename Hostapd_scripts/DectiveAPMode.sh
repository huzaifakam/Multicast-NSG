#!/bin/bash

cd
sudo printf "# interfaces(5) file used by ifup(8) and ifdown(8) \n auto lo \n iface lo inet loopback" | tee /etc/network/interfaces
sudo printf "#" | tee /etc/dnsmasq.conf
