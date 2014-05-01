/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (c) 2012-2014 Timothy Pearson
 * Raptor Engineering
 * http://www.raptorengineeringinc.com
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

// #define SERIAL_PORT "/dev/ttyAMA0"
#define SERIAL_PORT "/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A601LOWH-if00-port0"

int open_serial_port()
{
	struct termios oldtio, newtio;
	int tty_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NONBLOCK | O_APPEND);
	if (tty_fd < 0) {
		printf("[FAIL] Unable to open serial device %s\n\r", SERIAL_PORT); fflush(stdout);
		return 1;
	}

	tcgetattr(tty_fd, &oldtio);	// Save current port settings

	long serialBaud = B115200;

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = serialBaud | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	// Set input mode (non-canonical, no echo,...)
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0;	// Inter-character timer unused
	newtio.c_cc[VMIN]  = 0;	// Blocking read unused

	tcflush(tty_fd, TCIFLUSH);
	tcsetattr(tty_fd, TCSANOW, &newtio);

	int cc;
	char buffer[1];
	do {
		cc = read(tty_fd, buffer, 1);
	} while (cc > 0);

        return tty_fd;
}