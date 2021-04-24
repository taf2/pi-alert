# Monitor a Raspberry PI

Raspberry PI is great however in the wild it seems it can be a little flakey maybe due to bad power source or maybe other factors
but whatever the case the usecase for this feather is to watch out for our PI friend.  It can do this by listening for a small
process running on the PI that will enable HIGH or LOW on a specific PIN.  If the feather see's the PIN is not at the correct
state HIGH or LOW then the feather will send a reset signal to the PI either by cutting it's power or by sending a reset.

Additionally, because we're using an esp32 we have wifi so the feather can report back to a central hub the number of faults
and or current health status of the running PI whether the PI is alive or not...


# power settings for Raspberry PI 

add to  /etc/rc.local
/usr/bin/tvservice -o
echo gpio | sudo tee /sys/class/leds/led0/trigger
