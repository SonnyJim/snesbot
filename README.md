snesbot
=======

Controlling a SNES with a Pi.


The SNES controller protocol is fairly simple, every 16.67ms (or 60Hz) it sends out a latch pulse to the controller, which then clocks out 16 bits of data to the console.  Although the latch pulse is fairly slow, the clock is fairly quick, with 16us pulses.

I tried a few different approaches.  I first looked up some benchmarks for the GPIO on the Pi and found that it might be possible to bitbang the SNES directly.  That approach ended in failure, mainly due to the Pi not being able to clock out the data fast enough for the SNES.

I tried a different approach, where I took a knock off SNES controller and tried to wire up the GPIO directly to the pads, but this again ended in failure due to a combination of my shoddy soldering skills and the crapness of the PCBs, as traces were lifting everywhere.

I then tried my third approach, which was met with some success.  I duplicated the hardware inside of the SNES controller (2 x 4021 shift registers) and wired up the GPIO pins to that.  To my amazement it worked and passed the Nintendo Controller Test software.

I accidentally wired up the last 4 bits of the 2nd 4021 to GND rather than VCC, so some games would detect it wasn't an official controller, whoopsy daisy.

Hardware required:

A Raspberry Pi + SD card
2 x 4021 shift registers
An old floppy cable or something to break out the GPIO pins


TODO:

Record and playback button presses (in progress)
Netplay (depending on the RNG method used by each game)
TAS video playback (again, depending on the RNG method used)
