/*
##############################################################################
#	SNESBot - Pi controlled SNES Bot
#	https://github.com/sonnyjim/snesbot/
#
#	Copyright (c) 2013 Ewan Meadows
##############################################################################
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
##############################################################################
*/

// 			WiringPi	P1 Pin
#define B_Pin		 8 		// 3
#define Y_Pin		 9 		// 5
#define Select_Pin	 7 		// 7
#define Start_Pin	 15 		// 8
#define Up_Pin		 16 		// 10
#define Down_Pin	 0 		// 11
#define Left_Pin	 1 		// 12
#define Right_Pin	 2 		// 13
#define A_Pin		 3 		// 15
#define X_Pin		 4 		// 16
#define TLeft_Pin	 5 		// 18
#define TRight_Pin	 12 		// 19

#define Latch_Pin	 13 		// 21

//Set record buffer to 16MB
#define RECBUFSIZE	1024 * 1024 * 16 

//How many SNES buttons there are
#define NUMBUTTONS	12

//Magic file number
#define FILEMAGIC	0xBEC16260

//Size of each filepos chunk
#define CHUNK_SIZE (sizeof(unsigned long int) + sizeof(struct js_event))
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <signal.h>
#include <ctype.h>
