#pragma once

#define VERSION "0.1 Dev"

// Configuration
#define PKT_RATE 1 // Sample data every second
#define PKT_LEN 64
#define PKT_BURST 1 // 10 packets are sent at a time; past 10 seconds
#define TX_RATE 3 // Burst every 3 seconds
		  
// Debug things
#define USE_FAKE_CONTROLLER true
#define USE_IP
