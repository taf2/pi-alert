# Monitor a Raspberry PI

Raspberry PI is great however in the wild it seems it can be a little flakey maybe due to bad power source or maybe other factors
but whatever the case the usecase for this feather is to watch out for our PI friend.  It can do this by listening for a small
process running on the PI that will enable HIGH or LOW on a specific PIN.  If the feather see's the PIN is not at the correct
state HIGH or LOW then the feather will send a reset signal to the PI either by cutting it's power or by sending a reset.

Additionally, because we're using an esp32 we have wifi so the feather can report back to a central hub the number of faults
and or current health status of the running PI whether the PI is alive or not...


# power settings for Raspberry PI 

  - read: http://www.earth.org.uk/note-on-Raspberry-Pi-setup.html

add to  /etc/rc.local
/usr/bin/tvservice -o
echo gpio | sudo tee /sys/class/leds/led0/trigger

vim /boot/config.txt
force_turbo=0 #turns on frequency scaling
arm_freq=700 #sets max frequency
arm_freq_min=100 #sets min frequency

sudo update-rc.d dphys-swapfile disable

added to /etc/sysctl.conf
```
tail /etc/sysctl.conf
# Magic system request Key
# 0=disable, 1=enable all, >1 bitmask of sysrq functions
# See https://www.kernel.org/doc/html/latest/admin-guide/sysrq.html
# for what other values do
#kernel.sysrq=438

vm.dirty_background_ratio=20
vm.dirty_ratio=40
vm.dirty_writeback_centisecs=12011
vm.dirty_expire_centisecs=12101
```
