reset
sudo ip route add 224.0.67.67 dev wlp1s0
cd Desktop
cd send
make
sudo rmmod AP_FeedBack_sending.ko
sudo insmod AP_FeedBack_sending.ko
cd ../recv
make
sudo rmmod AP_FeedBack_recv.ko
sudo insmod AP_FeedBack_recv.ko
sudo dmesg -C
#iperf -c 224.0.67.67 -u -t 100 -i 1 -b 300M
