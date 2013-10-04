snesbot
=======

Controlling a SNES with a Pi.


The SNES controller protocol is fairly simple, every 16.67ms (or 60Hz for NTSC, 50Hz for PAL consoles) it sends out a latch pulse to each controller, which contain 2 x 4021 shift registers.  These shift registers then clock out the 16 bits of data to the console.  Although the latch pulse is fairly slow, the clock is fairly quick, with 16us between clock pulses.

I tried a few different approaches.  I first looked up some benchmarks for the GPIO on the Pi and found that it might be possible to bitbang the SNES directly.  That approach ended in failure, mainly due to the Pi not being able to clock out the data fast enough for the SNES.

I tried a different approach, where I took a knock-off SNES controller and tried to wire up the GPIO directly to the pads, but this again ended in failure due to a combination of my shoddy soldering skills and the crapness of the PCBs, as traces were lifting everywhere.

I then tried my third approach, which was met with some success.  I duplicated the hardware inside of the SNES controller (2 x 4021 shift registers) and wired up the GPIO pins to that.  I also needed a 74HCT244 to act as a buffer/level convertor for the latch pulse, as the output from the SNES is 5V logic whereas the GPIO pins on the Pi are only 3.3v tolerant.

To my amazement it worked and it's detected as a proper controller the Nintendo Controller Test software.

Current status of record/playback is that it works, although it's not presently accurate enough for most purposes.

Hardware required:

A Raspberry Pi + SD card + keyboard / joystick
2 x 4021 CMOS shift registers
1 x buffer / level convertor (I used a 74HCT244) for the latch pulse
Some perf/vero board to mount it all on
An old floppy cable or something to break out the GPIO pins
A knockoff/broken controller for the SNES connector

TODO:

Record and playback button presses (in progress)

Netplay (depending on the RNG method used by each game)

TAS video playback (again, depending on the RNG method used)

Some circuit diagrams and information about the controller protocol would be nice
