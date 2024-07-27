#!/bin/bash
sudo iptables --table nat --append POSTROUTING --out-interface wlp0s20f3 -j MASQUERADE
sudo iptables --append FORWARD --in-interface wlp0s20f3 -j ACCEPT
sudo echo 1 > /proc/sys/net/ipv4/ip_forward
