#!/bin/csh

# device permissions
chmod go+rw /dev/ttyS0 /dev/ttyS1 /dev/fd0

# particle account ammenities
chsh -s /bin/csh particle
cat - >! /home/particle/.rhosts <<EOF
128.151.144.36 ksmcf
stomp.pas.rochester.edu ksmcf
128.151.144.148 ksmcf
mcfarland.pas.rochester.edu ksmcf
131.225.232.170 ksmcf
mcfarland.fnal.gov ksmcf
EOF
chown particle.particle /home/particle/.rhosts
chmod go-rw /home/particle/.rhosts

echo /dev/ttyS0 >! /home/particle/.particle-port
chown particle.particle /home/particle/.particle-port
chmod -w /home/particle/.particle-port

# adding rc.local support for turning off echo
set rcmod = `grep -c /bin/stty /etc/rc.d/rc.local`
if ( $rcmod == 0 ) then
  cp /etc/rc.d/rc.local /etc/rc.d/rc.local.orig
  echo '/bin/setserial /dev/ttyS0 auto_irq autoconfig' >> /etc/rc.d/rc.local
  echo '/bin/setserial /dev/ttyS1 auto_irq autoconfig' >> /etc/rc.d/rc.local
  echo '/bin/stty -F /dev/ttyS0 -echo' >> /etc/rc.d/rc.local
  echo '/bin/stty -F /dev/ttyS1 -echo' >> /etc/rc.d/rc.local
endif

# adding rc.local support for synchronizing clock on boot
#set rcmod = `grep -c ntpdate /etc/rc.d/rc.local`
#if ( $rcmod == 0 ) then
#  echo '/usr/sbin/ntpdate sundial.columbia.edu; /sbin/hwclock --systohc' >> /etc/rc.d/rc.local
#endif

# add user floppy device (/mnt/qfloppy) and usbflash device to /etc/fstab
mkdir -p /mnt/qfloppy
set fsmod = `grep -c qfloppy /etc/fstab`
mkdir -p /mnt/usbflash
set fsmod2 = `grep -c usbflash /etc/fstab`
if ( $fsmod == 0 || $fsmod2 == 0 ) then
  cp /etc/fstab /etc/fstab.orig
  if ( $fsmod == 0 ) then
    echo '/dev/fd0 /mnt/qfloppy auto noauto,user 0 0' >> /etc/fstab
  endif
  if ( $fsmod2 == 0 ) then
    echo '/dev/sda1 /mnt/usbflash auto noauto,user,owner,rw 0 0' >> /etc/fstab
  endif
endif

# synchronize clock for further work
#/usr/sbin/ntpdate sundial.columbia.edu; /sbin/hwclock --systohc

