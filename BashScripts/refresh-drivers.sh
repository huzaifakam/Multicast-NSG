cd /usr/src/linux-3.13.2
make drivers/net/wireless/ath/ath9k -j 3
make modules SUBDIRS=drivers/net/wireless/ath/ath9k -j 3

echo REMOVING OLD MODULES

rm /lib/modules/3.13.2/kernel/drivers/net/wireless/ath/ath9k/*.ko

cp drivers/net/wireless/ath/ath9k/*.ko /lib/modules/3.13.2/kernel/drivers/net/wireless/ath/ath9k/

echo SUCCESSFULLY REPLACED MODULES
modprobe -r ath9k
modprobe ath9k

killall wpa_supplicant