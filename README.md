# MRDC xbox stuff, 64 bit Linux based program:

Program to connect to arduino/bluetooth and send appropriate xbox controller input. Use make to compile, there may be an issue with the .so library not being properly linked, in that case have to add the path to file to the ld config file using:

    /home/[user]/mrdc/gattlib/lib/libagattlib.so > /etc/ld.so.conf.d/local.conf

May not be right way, some random late night work to make this happen. Note: make clean will fail if it wasn't compiled, can just run `rm mrdc` when want it removed instead tbh.

If other compiler errors happen, lookup dependencies for the libraries to install and try to compile again. If running on a different OS/not 64 bit linux, get the appropriate gattlib library and replace.

## mrdc.c
Main program to seend to the beetle ble arduino, using xbox input and sending bytes to arduino. Xbox controller bluetooth range too low, use laptop's bluetooth instead.

## beetleSketch
Sketch for the beetle ble arduino, takes in the input through bluetooth from the laptop.

## nanoSketch
Controls the relays on the robot + gets some data from the beetle arduino due to having more pwm pins.