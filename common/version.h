#pragma once

#define VERSION "0.1 Dev"

// Configuration
#define PKT_RATE 1 // Rate to sample data from motor controller
#define PKT_LEN 64 // Length of one packet (NOT TLink frame). This is fixed
#define PKT_BURST 1 // Number of packets to send during each tx
#define TX_RATE 1 // Number of seconds between each tx
		  
// Debug things
#define USE_FAKE_CONTROLLER
#define USE_FAKE_GPS
//#define USE_IP
