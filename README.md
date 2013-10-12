snesbot
=======

Controlling a SNES with a Pi.
-----------------------------

The main aim of the project is to:

A.  Use USB devices connected to the Pi (joysticks etc) to control a real SNES

B.  Record and playback inputs into a real SNES

How it works:
-------------
The SNES controller protocol is fairly simple, every 16.67ms (or 60Hz for NTSC, 50Hz for PAL consoles) it sends out a latch pulse to each controller, which contain 2 x 4021 shift registers.  These shift registers then clock out the 16 bits of data to the console.  Although the latch pulse is fairly slow, the clock is fairly quick, with 16us between clock pulses.

So far so good, but what about randomness in games?  Well, the good thing is that the SNES lacks a source of entropy (hardware random number generator or even a realtime clock), so most games use an absurdly simple principle.  Count the number of latches before a controller input is pressed and use that to see a RNG.  Also bear in mind that the SNES doesn't start sending out the latch pulses until the CPU is working correctly, this means that as long as we send out the same button presses on exactly the same latch on each poweron, then our RNG will act *exactly* the same, with the same enemy patterns, powerups, items etc.

My different approaches:
-----------------------

I tried a few different approaches.  I first looked up some benchmarks for the GPIO on the Pi and found that it might be possible to bitbang the SNES directly.  That approach ended in failure, mainly due to the Pi not being able to clock out the data fast enough for the SNES when it sees the latch pulse.  

The way around this was to use some chips that can handle the faster clock pulse and send the data out to the SNES controller port.

I took a cheap knock-off SNES controller (which also supplied the connector I needed for the controller port) and tried to wire up the GPIO directly to the pads, but this again ended in failure due to a combination of my shoddy soldering skills and the crapness of the PCBs, as traces were lifting everywhere.

I then tried my third approach, which was met with some success.  I duplicated the hardware inside of the SNES controller (2 x 4021 shift registers) and wired up 12 GPIO pins to that (X, Y, B, A, L, R, Up, Down, Left and Right).  

I also needed something to act as a buffer/level convertor for the latch pulse, as the output from the SNES is 5V logic whereas the GPIO pins on the Pi are only 3.3v tolerant.

To my amazement it worked and it was detected as a proper controller the Nintendo Controller Test software!

Live input was working fine, both from a USB keyboard and a PS1 controller connected via a USB adapter, but recording and playback would lose sync after a few button presses, which was slightly disheartening.  Perhaps all those people telling me that I wouldn't be able to make it work on a Pi were right and I've have to change my approach again to use a microcontroller instead.

After a bit more playing around I had a bit of a eureka moment, where I found out that I was supplying the 4021's with 3.3V instead of 5V.  Also I hadn't put in a blocking diode to stop current flow back into the SNES.  Once I had fixed both of these problems playback worked a *lot* better.

Current status of record/playback is that it works, although it loses sync after a couple of minutes.  I've tested it with:

Hardware required:
-----------------
A Raspberry Pi + SD card + keyboard / joystick

2 x CD4021B CMOS shift registers

1 x buffer / level convertor for the latch pulse

Some perf/vero board to mount it all on

An old floppy cable or something to break out the GPIO pins

A knockoff/broken controller for the SNES connector


Games confirmed to be working:
------------------------------
USA UN Squadron

Super Mario Kart

Wagyan Paradise

Killer Instinct PAL

Super Mario All-Stars JPN

SuperGB JPN + Tetris DX

SuperGB JPN + Tetris

Super R-Type

Features:
---------
Live input via USB joystick or keyboard attached to the Pi

Record joystick inputs and playback

Tool Assisted Video playback via lsnes and Ilari's polldump.lua script


To Playback a lsnes TAS video:
------------------------------
Download lsnes video from TASVideos.org (lsmv)

Download lsnes emulator and install

Download polldump.lua script

Start lsnes emulator, load ROM, pause and reset console

Load lua script

Type in the messages window:

L start_dump ("filename.dump")

and press 'Execute'

Start playback

At the end of playback, type:

L end_dump ()

and press 'Execute'

Copy the dump file to the Pi

Load with:

sudo ./snesbot -l -L -p -j -f filename.dump

Turn on SNES and hopefully watch it do incredible things

TODO:
----
Netplay (depending on the RNG method used by each game)

Support other TAS video files other than lsnes

Right now the code is heavily based around using a USB PS1 controller, would be nice to support other controllers

Add support for SNES mouse via a USB mouse attached to the Pi.

Autofire/special moves on a single button press
