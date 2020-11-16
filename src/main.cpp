/**
*  Govee BLE gateway.
*  Full BLE scanner with govee manufacturer packet interpretation
*  to decode temperature, humidity and battery data
*  and logging to MQTT and/or InfluxDB.
*/

#include <cstdlib>
#include <csignal>
#include <iostream>
#include <string>
#include <map>
#include <queue>
#include <sys/ioctl.h>
#include <locale>
#include <iomanip>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

#include "bleScan.h"
#include "blePacket.h"
#include "govee-logger.h"

//! Scanning flag
volatile bool isScanning = true;

//! Signal handler for SIGINT
void SignalHandlerSIGINT(int signal)
{
	(void) signal;
	isScanning = false;
	std::cerr << "SIGINT caught." << std::endl;
}

//! Signal handler for SIGHUP
void SignalHandlerSIGHUP(int signal)
{
	(void) signal;
	isScanning = false;
	std::cerr << "SIGHUP caught." << std::endl;
}

//! Main
int main(void)
{

	// signal handlers
	typedef void(*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandlerSIGINT = signal(SIGINT, SignalHandlerSIGINT);
	SignalHandlerPointer previousHandlerSIGHUP = signal(SIGHUP, SignalHandlerSIGHUP);

	// Data
	BLEScan gble;												//!< BLEScan object
	std::map<bdaddr_t,int> goveeMap;		//!< Map for known Govee devices
	isScanning = true;									//!< Scanning flag

	std::cout << "------ STARTED ----- " << std::endl;

	// initialize the logging module
	Govee_logger gl("govee-gateway.ini");

	// connect to BLE and start scanning
	if (gble.connect())
	{
		while (isScanning)
		{
			BLEPacket bp;
			if (gble.scan(&bp))
			{
				// print packet info if it's in the list of Govee devices
				if ((goveeMap.find(bp.bdaddr) != goveeMap.end()))
				{
					bp.printInfo(gl.debugLevel);
				}

				// see if we have manufacturer data
				std::map<int,BLEPacket::t_adStructure>::iterator it = bp.adStructures.find(0xff);
				if (it!=bp.adStructures.end())
				{
					// do we have a packet starting with 0x88EC? If so, we likely have a Govee sensor
					if (it->second.data[0]==0x88 && it->second.data[1]==0xEC)
					{
						// add to govee map if it doesn't exist yet;
						if (goveeMap.find(bp.bdaddr) == goveeMap.end())
						{
							bp.printInfo(gl.debugLevel);
							goveeMap.insert(std::make_pair(bp.bdaddr,1));
						}
						// log data
						gl.logData(&bp,it->second.data);
					} // 088EC
				} // manufacturer info

				usleep(100);
			} // scan
		} // while
		gble.disconnect();
	} else {
		std::cerr << "Could not connect to BLE device." << std::endl;
	}

	signal(SIGHUP, previousHandlerSIGHUP);
	signal(SIGINT, previousHandlerSIGINT);

}
