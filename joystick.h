/*
 * piSnes.h:
 *	Driver for the SNES Joystick controller on the Raspberry Pi
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with wiringPi.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#define	MAX_SNES_JOYSTICKS	8

#define	SNES_B		0x8000
#define	SNES_Y		0x4000
#define	SNES_SELECT	0x2000
#define	SNES_START	0x1000
#define	SNES_UP		0x0800
#define	SNES_DOWN	0x0400
#define	SNES_LEFT	0x0200
#define	SNES_RIGHT	0x0100
#define	SNES_A		0x0080
#define	SNES_X		0x0040
#define	SNES_L		0x0020
#define	SNES_R		0x0010
#define	SNES_BIT14	0x0008
#define	SNES_BIT15	0x0004
#define	SNES_BIT16	0x0002

#ifdef __cplusplus
extern "C" {
#endif

extern int          detectSnesJoystick (int joystick) ;
extern int          setupSnesJoystick (int dPin, int cPin, int lPin) ;
extern unsigned short int  readSnesJoystick (int joystick) ;

#ifdef __cplusplus
}
#endif
