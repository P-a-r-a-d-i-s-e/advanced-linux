#!/bin/sh

echo "Hello from rootfs!"

HOSTNAME=zero
VERSION=0.0.1
hostname $HOSTNAME
mount -n -t proc proc /proc
mount -n -t sysfs sysfs /sys
mount -n -t tmpfs mdev /dev
mdev -s

# Set PATH
export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin

# Start other daemons
/sbin/syslogd
