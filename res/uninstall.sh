#!/bin/sh
# sms-forward device uninstall script

systemctl stop sms-forward
systemctl disable sms-forward

rm -f /lib/systemd/system/sms-forward.service
rm -f /home/root/sms/sms-forward
rm -f /home/root/sms/sms-forward-daemon
rm -f /home/root/sms/sms-forward-reload

systemctl daemon-reload

echo "sms-forward uninstalled"
echo "to remove config file, run: "
echo "  rm -f /home/root/sms/config.json"

rm -f /home/root/sms/uninstall.sh
