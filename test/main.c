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
 * (c) 2014 Audrey Pearson <aud.pearson@gmail.com>
 * (c) 2014 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <timer.h>

// #define KP 15 / 10
// #define KI 5 / 10
// #define KD 10 / 10

#define KP_FORWARD 5
#define KI_FORWARD 0.3
#define KD_FORWARD 0.0

#define KP_TURN 0.75
#define KI_TURN 0.0
#define KD_TURN 0.0

#define X_CENTER 80
#define Y_CENTER 60
#define X_TRACK (X_CENTER)
#define Y_TRACK (Y_CENTER - 40)

// #define TIMER_INTERVAL_MSECS 100
#define TIMER_INTERVAL_MSECS 50
//#define TIMER_INTERVAL_MSECS 10

//function declarations
void timer_handler(void);
int turn_pid(int error, char reset_integral);
int forward_pid(int error, char reset_integral);
int open_serial_port();
int open_usb_serial_port();
void motor_control(int turn_drive, int forward_drive);
void vision_data_receiver(int *horiz_err, int *vert_err, char* locked);

// global variables and arrays
int var = 0;
int tty_fd;
int tty_fd_usb;
char tx_data[3]; //3-byte array to send data

int blob_size;
int blob_size_lower;

char rx_data[60]; //60-byte array to receive vision data
int rx_data_ptr = 0;

int main()
{
	char tx_data[10]; //create 10-byte array to send data

	tty_fd = open_serial_port(); // (tty = teletype file descriptor)
	tty_fd_usb = open_usb_serial_port(); // secondary port uses USB

	//---send configuration settings to vision system
	// CR noise floor threshold
	tx_data[0] = 65; // send command to change CR noise floor threshold to following value
	tx_data[1] = 13;
	tx_data[2] = 90; // CR noise floor threshold value
	tx_data[3] = 13; //send required carriage return
	write(tty_fd, tx_data, 4); //sends the three previous bytes

	// Y noise floor threshold
	tx_data[0] = 73; //set value to the following
	tx_data[1] = 13;
	tx_data[2] = 90; //value
	tx_data[3] = 13; //carriage return
	write(tty_fd, tx_data, 4);

	// Cb noise floor threshold
	tx_data[0] = 66;
	tx_data[1] = 13;
	tx_data[2] = 90;
	tx_data[3] = 13;
	write(tty_fd, tx_data, 4);

	//Color similarity threshold
	tx_data[0] = 72;
	tx_data[1] = 13;
	tx_data[2] = 15;
	tx_data[3] = 13;
	write(tty_fd, tx_data, 4);

	// miminum blob size
	tx_data[0] = 78;
	tx_data[1] = 13;
	tx_data[2] = 15;
	tx_data[3] = 13;
	write(tty_fd, tx_data, 4);

	// HSV values
	// Measured
	tx_data[0] = 91;
	tx_data[1] = 13;
	tx_data[2] = 247;
	tx_data[3] = 13;
	tx_data[4] = 165;
	tx_data[5] = 13;
	tx_data[6] = 6;
	tx_data[7] = 13;
	write(tty_fd, tx_data, 8);

	// HSV values
	// Guesstimated saturation
	tx_data[0] = 92;
	tx_data[1] = 13;
	tx_data[2] = 247;
	tx_data[3] = 13;
	tx_data[4] = 150;
	tx_data[5] = 13;
	tx_data[6] = 6;
	tx_data[7] = 13;
	write(tty_fd, tx_data, 8);

	// HSV values
	// Guesstimated saturation
	tx_data[0] = 93;
	tx_data[1] = 13;
	tx_data[2] = 247;
	tx_data[3] = 13;
	tx_data[4] = 135;
	tx_data[5] = 13;
	tx_data[6] = 6;
	tx_data[7] = 13;
	write(tty_fd, tx_data, 8);

	// HSV values
	// Guesstimated saturation
	tx_data[0] = 94;
	tx_data[1] = 13;
	tx_data[2] = 247;
	tx_data[3] = 13;
	tx_data[4] = 180;
	tx_data[5] = 13;
	tx_data[6] = 6;
	tx_data[7] = 13;
	write(tty_fd, tx_data, 8);

	// configure system to use HSV mode
	tx_data[0] = 75;
	tx_data[1] = 13;
	write(tty_fd, tx_data, 2);

	// activate online tracking mode
	tx_data[0] = 52;
	tx_data[1] = 13;
	write(tty_fd, tx_data, 2);


	if(start_timer(TIMER_INTERVAL_MSECS, &timer_handler))
	{
		printf("\n timer error\n");
		return(1);
	}

	printf("\npress ctl-c to quit.\n");

	while(1) {
		sleep(100000);
	}

	stop_timer();

  return(0);
}

//**************************************************
//------FUNCTIONS
//**************************************************

void vision_data_receiver(int *horiz_err, int *vert_err, char* locked) {
	int horiz_coord;
	int vert_coord;
	int cc;
	char buffer[1];
	do {
		cc = read(tty_fd, buffer, 1);

		//data received
		if (cc > 0) {
			if (rx_data_ptr == 0) {
				if(buffer[0] != 176) {
					printf("Ooops! %d\n\r", buffer[0]);
					continue;	//safety--does not store data if first byte is not the expected value 176
				}
			}
			rx_data[rx_data_ptr] = buffer[0];
			rx_data_ptr++;
		}
	} while ((cc > 0) && (rx_data_ptr < 28)); //keeps receiving characters and storing them in the rx_data array
// 	printf("rx_data_ptr: %d\n\r", rx_data_ptr);

	if (rx_data_ptr >= 28) {
		int i;
		for(i=0; i<27; i++) {
			printf("%d ", rx_data[i]);
		}
		printf("\n\r");
		// Processing: store just-read data in appropriate variables
		horiz_coord = rx_data[2]; //get horizontal coordinate
		vert_coord = rx_data[3]; //get vertical coordinate
		if ((horiz_coord == 0) && (vert_coord == 0))
			*locked = 0;
		else
			*locked = 1;
		//concatenate upper and lower blob size bytes:
		blob_size_lower = rx_data[15];
		blob_size = rx_data[14]; //store high bits
		blob_size << 8; //bitshift to make room for lower byte
		blob_size = blob_size | blob_size_lower; //blob size now contains all 16 bits

		// neg horiz_err means turn left. pos horiz_err means turn right
		*horiz_err = X_TRACK - horiz_coord;
		*vert_err = vert_coord - Y_TRACK;

		rx_data_ptr = 0;
	}
}

void timer_handler(void) {
	static int vert_err;
	static int horiz_err;
	int forward_drive;
	int turn_drive;
	char locked;

	vision_data_receiver(&horiz_err, &vert_err, &locked);

	if (locked == 1) {
		turn_drive = turn_pid(horiz_err, 0);
		forward_drive = forward_pid(vert_err, 0);
	}
	else {
		turn_drive = turn_pid(0, 1);
		forward_drive = forward_pid(0, 1);
	}

// 	turn_drive = turn_pid(30);
// 	forward_drive = forward_pid(60);
	
// 	printf("%d %d\n\r", horiz_err, vert_err);
// 	printf("%d %d\n\r", turn_drive, forward_drive);
	motor_control(turn_drive, forward_drive);
}

void motor_control(int turn_drive, int forward_drive) {
	int uncorrected_right_motor_output;
	int uncorrected_left_motor_output;
	int right_motor_output;
	int left_motor_output;

	uncorrected_right_motor_output = forward_drive - turn_drive;
	uncorrected_left_motor_output = forward_drive + turn_drive;

	//forwad drive and turn drive can take values between -127 to 127.
	if (uncorrected_right_motor_output < -127) {
		right_motor_output = -127;
	} else if (uncorrected_right_motor_output > 127) {
		right_motor_output = 127;
	}
	else {
	 	right_motor_output = uncorrected_right_motor_output;
	}

	if (uncorrected_left_motor_output < -127) {
		left_motor_output = -127;
	}
	else if (uncorrected_left_motor_output > 127) {
		left_motor_output = 127;
	}
	else {
	 	left_motor_output = uncorrected_left_motor_output;
	}

	left_motor_output = left_motor_output*(-1); //invert left motor output
	right_motor_output = right_motor_output + 128;
	left_motor_output = left_motor_output + 128;
	if (right_motor_output > 254) {
		right_motor_output = 254;
	}
	if (left_motor_output > 254) {
		left_motor_output = 254;
	}

	tx_data[0] = 255; //sync byte
	tx_data[1] = right_motor_output;
	tx_data[2] = left_motor_output;
	write(tty_fd_usb, tx_data, 3);

// 	printf("%d %d %d\n\r", tx_data[0], tx_data[1], tx_data[2]);

}

// PID loops
int turn_pid (int error, char reset_integral) {
	signed int delta_err;
	signed int p_out;
	signed int i_out;
	signed int d_out;
	signed int output;

	static signed int integral_err;
	static signed int prev_err;

	if (reset_integral) {
		integral_err = 0;
	}

	delta_err = prev_err - error;
	integral_err += error;

	if (integral_err > 200)
		integral_err = 200;
	if (integral_err < -200)
		integral_err = -200;

	p_out = error * KP_TURN;
	i_out = integral_err * KI_TURN;
	d_out = delta_err * KD_TURN;

	output = p_out + i_out + d_out;
	if (output > 127)
		output = 127;
	if (output < -127)
		output = -127;

	prev_err = error;

// 	printf("PID turn: %d %d %d\n\r", p_out, i_out, d_out);
	
	return output;
}

int forward_pid(int error, char reset_integral) {
	signed int delta_err;
	signed int p_out;
	signed int i_out;
	signed int d_out;
	signed int output;

	static signed int integral_err;
	static signed int prev_err;

	if (reset_integral) {
		integral_err = 0;
	}

	delta_err = prev_err - error;
	integral_err += error;

	if(integral_err > 200)
		integral_err = 200;
	if(integral_err < -200)
		integral_err = -200;

	p_out = error * KP_FORWARD;
	i_out = integral_err * KI_FORWARD;
	d_out = delta_err * KD_FORWARD;

	output = p_out + i_out + d_out;
	if(output > 127)
		output = 127;
	if(output < -127)
		output = -127;

	prev_err = error;
	
// 	printf("PID fwd: %d %d %d\n\r", p_out, i_out, d_out);

	return output;
}






