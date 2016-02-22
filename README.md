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

So far so good, but what about randomness in games?  Well, the good thing is that the SNES lacks a source of entropy (hardware random number generator or even a realtime clock), so most games use an absurdly simple principle.  Count the number of latches before a controller input is pressed and use that to see a PRNG.  Also bear in mind that the SNES doesn't start sending out the latch pulses until the CPU is working correctly, this means that as long as we send out the same button presses on exactly the same latch on each poweron, then our PRNG will act *exactly* the same, with the same enemy patterns, powerups, items etc.

My different approaches:
-----------------------

I tried a few different approaches.  I first looked up some benchmarks for the GPIO on the Pi and found that it might be possible to bitbang the SNES directly.  That approach ended in failure, mainly due to the Pi not being able to clock out the data fast enough for the SNES when it sees the latch pulse.  

The way around this was to use some chips that can handle the faster clock pulse and send the data out to the SNES controller port.

I took a cheap knock-off SNES controller (which also supplied the connector I needed for the controller port) and tried to wire up the GPIO directly to the pads, but this again ended in failure due to a combination of my shoddy soldering skills and the crapness of the PCBs, as traces were lifting everywhere.

I then tried my third approach, which was met with some success.  I duplicated the hardware inside of the SNES controller (2 x 4021 shift registers) and wired up 12 GPIO pins to that (X, Y, B, A, L, R, Up, Down, Left and Right).  

I also needed something to act as a buffer/level convertor for the latch pulse, as the output from the SNES is 5V logic whereas the GPIO pins on the Pi are only 3.3v tolerant.

To my amazement it worked and it was detected as a proper controller the Nintendo Controller Test software!

Live input was working fine, both from a USB keyboard and a PS1 controller connected via a USB adapter, but recording and playback would lose sync after a few button presses, which was slightly disheartening.  Perhaps all those people telling me that I wouldn't be able to make it work with the overhead of Linux were right and I've have to change my approach again to use a microcontroller instead.

After a bit more playing around I had a bit of a eureka moment, where I found out that I was supplying the 4021's with 3.3V instead of 5V.  Also I hadn't put in a blocking diode to stop current flow back into the SNES.  Once I had fixed both of these problems playback worked a *lot* better.  I'm not quite sure why live input worked and recorded didn't at 3.3V, but I'm certainly glad it's working at 5V!


Flash forward 2 years......
---------------------------

I picked up the project again after getting back into SNES collecting and decided to try another way.  I wasn't really happy with using so much of the Pi's GPIO, so I explored some other options.  The author of the WiringPi library pointed me in the direction of the MCP23017 port expander chip.  The good thing about this chip is that it allows 16bits of input/ouput at 5V and can communicate with the Pi using I2C, which is 3.3v.  This is a very good thing, as it allows me to drive 16bits of data at 5V using only 2 lines from the Pi (SDA/SCL), which will be at 3.3v.  No more level shifter chips, huzzah (well nearly).  I still need to level shift the latch input from the SNES into the Pi, but I can do that very simply with a voltage divider.  It also means that to add another player it means just adding another 3 chips (1 x MCP23017, 2 x 4021's) and can use the same I2C line from the Pi, just set to different addresses.

Reading SNES joysticks
----------------------

Now that I had some spare GPIOs, I may as well make some use of them!  In the wiringPi library there is support for reading NES joysticks.  This is a good thing, as the NES joystick protocol is identical to the SNES one, just that the SNES protocol uses more bits for the extra buttons.  Another bonus is that all of the SNES joysticks I tried would run quite happily at 3.3v, meaning I could hook them straight into the Pi without needing to level shift them.  Double result!

This means that I can now operate the Pi in 'Passthrough' mode, where I read the SNES joystick inputs into the Pi, then load up the shift registers with that data using the MCP23017 which is then clocked into the SNES.  Because I am intecepting the joystick inputs I can:

* Hide the Pi inside the SNES case
* Use a ribbon cable to pass the controller data into the Pi and out again into the SNES, no more messy soldering onto the back of the SNES controller input block.

TAS Videos
----------

So I was at the stage where I could playback from my own playback format and was successful with roughly 90% of the games I tried.  So it was time to look at working on using emulator movie files.  These are files recorded by the emulator that are used to playback high score attempts/speed runs etc.  Sometimes emulators are used to create bizarre and lightning quick videos called 'Tool Assisted Speedruns'.  These would be a good source of videos to demonstrate the abilities of SNESBot.  To try and get things to work first time, I went the simple route.  I chose a TAS video that had previously been verifed on hardware, which was the KFC-Mario speedrun of Super Mario All-Stars: Super Mario Bros. Lost levels.  Because I was intending to run on hardware, I would need to use a pretty accurate emulator, so lsnes looked like a good candidate.  I found a Lua script by Ilari to output the controller data I needed for the emulator and wrote some importing code for my bot.  After a few false starts, ILari was particulary helpful in getting it working.  So I was at the stage where I coulld give this TAS video a try.

I grabbed my copy of SMAS, plugged it in and it went through the title screens, Mario started to run and then.

Nothing.  He just sat there hopping on the spot and generally not doing what I wanted him to do.

This was slightly annoying, so I tried it with the console in PAL rather than NTSC, still the same.  I took my SNES apart and saw the one of the legs of PPU2 had snapped off after I installed a SuperCIC mod.  I believe this was causing it to run in permenant PAL mode, throwing off the timings.  With a bit of delicate surgery, I managed to get it soldered back on, setup again and hit start.  And I watched Mario go absolutely nuts for 33 minutes, flying impossibly through levels, then destroy Bowser and complete the game.

Feeling rather pleased. my next stop was to write an importer for SNES9X emulator movie files, which hasn't worked out so well.  Which is a shame, as was a popular file format, with a lot of TAS runs available.  Because the TAS videos rely on very sharp timing, if the emulation isn't exactly the same as a SNES, the frames 'desync' and the Pi/SNES lose sync with each other.

This normally happens during a 'lag frame', which is a frame where the SNES CPU is too busy to poll the joystick inputs, because it's too busy drawing enemies exploding etc.  The reason lsnes input files work and SNES9x files don't is down to the way they emulate these lag frames.

I really need to get myself a logic analsyer as so far I've been doing this 'blind'.


Hardware required:
------------------
A Raspberry Pi + SD card + USB keyboard / joystick

1 x MCP23017 Port Expander
2 x CD4021B CMOS shift registers

1 x buffer / level convertor for the latch pulse

Some perf/vero board to mount it all on

An old floppy cable or something to break out the GPIO pins

A knockoff/broken controller for the SNES connector

Software needed:
----------------
Raspbian
WiringPi library by gordonDrogon

Games confirmed to be working:
------------------------------
Assault Suit Vulcan

Contra 3: The Alien Wars

Earthworm Jim

UN Squadron

Super Mario Kart

Wagyan Paradise

Killer Instinct

Street Fighter 2

Street Fighter 2: Turbo

Super Mario All-Stars

Super Mario Kart

Super Mario World 2: Yoshis Story

SuperGB JPN + Tetris DX

SuperGB JPN + Tetris

Super R-Type

Games that don't work
---------------------

NBA Jam

Super Tennis

Speedy Gonzales

Why they don't work is a bit of a mystery to me at the moment.  As the code that reads the controller interrupts isn't the same for each game, it could be that my bot isn't precise enough with the timings.  Also some games use uninitialised memory as the seed for the PRNG, which means the game won't be exactly the same with the same inputs.  I'll have to run through them with a SNES debugger at some point

Features:
---------

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

I have included a script for snes9x-rr, but unfortunately it seems that the emulation isn't accurate enough, especially if there are 'lag frames' during ganmeplay.

TODO:
----
Live input via USB joystick or keyboard attached to the Pi
Netplay (depending on the RNG method used by each game)

Support other TAS video files other than lsnes

Add support for SNES mouse via a USB mouse attached to the Pi.

Autofire/special moves on a single button press

Find out why some games don't work
