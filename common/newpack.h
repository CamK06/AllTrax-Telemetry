#pragma once
// New packet protocol; AX.25 based:
//
// Frame format; second row is nr bytes:
// [flag][CarID][ACK][DataType][Bytes][Data][FCS][flag]
// [ 2  ][  1  ][ 1 ][   1    ][  1  ][<256][ 2 ][  1 ]
//
// - FCS will be calculated same as AX.25
// - Flag is the same as AX.25
// - ACK field is either true or false; whether or not an ACK is requested from receiver
// - Max data bytes is 256
// - Car ID is just number of the car
// - BURST packets will likely NOT be supported
//
// Data Types:
// 1 - Sensor telemetry
// 2 - Computer telemetry
// 3 - Command
// 4 - Command response
//
// Commands: *this might not be implemented
// CHSR [RATE] - Change data sample rate 
// CHTR [RATE] - Change data tx rate
// START - Start telemetry transmit loop
// STOP - Stop transmitting telemetry
// EXEC [CMD] - Execute a shell command

