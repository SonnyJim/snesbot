#
# Makefile:
#	SNESBot - Pi controlled SNES Bot
#	https://github.com/sonnyjim/snesbot/
#
#	Copyright (c) 2013-2016 Ewan Meadows
#################################################################################
# This file is part of SNESBot:
#
#    SNESBot is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Lesser General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    SNESBot is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public License
#    along with SNESBot.  If not, see <http://www.gnu.org/licenses/>.
#################################################################################


DEBUG	= -g -O0
CC	= gcc
INCLUDE	= -I/usr/local/include
#OPT 	= -O3
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe -std=c99 $(OPT)
LDFLAGS	= -L/usr/local/lib
LDLIBS	= -lwiringPi -lwiringPiDev
OBJS = snesbot.o record.o joystick.o cfg.o isapi.o pidfile.o subs.o macro.o
all: snesbot

snesbot: $(OBJS)
	@echo [link]
	@$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf *.o snesbot
