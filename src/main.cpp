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
#include <getopt.h>

#include "bleScan.h"
#include "blePacket.h"
#include "govee-logger.h"

//! Scanning flag
volatile bool isScanning = true;

//! command line options
static const char short_options[] = "hl:c:i:v:x";
static const struct option long_options[] = {
		{ "help",   no_argument,       NULL, 'h' },
		{ "config",required_argument, NULL, 'c' },
		{ "interval",required_argument, NULL, 'i' },
		{ "verbosity",required_argument, NULL, 'v' },
		{ 0, 0, 0, 0 }
};
static void usage(int argc, char **argv)
{
	(void) argc;
	std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
	std::cout << "  Options:" << std::endl;
	std::cout << "    -h | --help               print this message" << std::endl;
	std::cout << "    -c | --config <file>      path to the config file" << std::endl;
	std::cout << "    -i | --interval <seconds> reporting interval in seconds" << std::endl;
	std::cout << "    -v | --verbosity <level>  verbosity level" << std::endl;
	std::cout << std::endl;
}

//! Signal handler for SIGINT
void SignalHandlerSIGINT(int signal)
{
	(void) signal;
	isScanning = false;
	std::cout << "SIGINT caught. Exiting." << std::endl;
}

//! Signal handler for SIGHUP
void SignalHandlerSIGHUP(int signal)
{
	(void) signal;
	isScanning = false;
	std::cerr << "SIGHUP caught. Exiting." << std::endl;
}

//! Main
int main(int argc, char **argv)
{

	// signal handlers
	typedef void(*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandlerSIGINT = signal(SIGINT, SignalHandlerSIGINT);
	SignalHandlerPointer previousHandlerSIGHUP = signal(SIGHUP, SignalHandlerSIGHUP);

	char* confFileName;
	confFileName = strdup("/etc/govee-gateway.conf");

	// handle command line parameters
	int verbosity=-1;
	int logInterval=-1;
	for (;;)
	{
		int idx;
		int c = getopt_long(argc, argv, short_options, long_options, &idx);
		if (c == -1)
			break;
		switch (c)
		{
		case 0: /* getopt_long() flag */
			break;
		case 'h':
			usage(argc, argv);
			exit(EXIT_SUCCESS);
		case 'c':
			confFileName = strdup(optarg);
			std::cout << "config is " << confFileName << std::endl;
			break;
		case 'i':
			try { logInterval = std::stoi(optarg); }
			catch (const std::invalid_argument& ia) { std::cerr << "Invalid argument: " << ia.what() << std::endl; exit(EXIT_FAILURE); }
			catch (const std::out_of_range& oor) { std::cerr << "Out of Range error: " << oor.what() << std::endl; exit(EXIT_FAILURE); }
			break;
		case 'v':
			try { verbosity = std::stoi(optarg); }
			catch (const std::invalid_argument& ia) { std::cerr << "Invalid argument: " << ia.what() << std::endl; exit(EXIT_FAILURE); }
			catch (const std::out_of_range& oor) { std::cerr << "Out of Range error: " << oor.what() << std::endl; exit(EXIT_FAILURE); }
			break;
		default:
			usage(argc, argv);
			exit(EXIT_FAILURE);
		}
	}

	// Data
	BLEScan gble;												//!< BLEScan object
	std::map<bdaddr_t,int> goveeMap;		//!< Map for known Govee devices
	isScanning = true;									//!< Scanning flag

	// initialize the logging module
	Govee_logger gl(confFileName);
	if (verbosity>-1) gl.verbosity=verbosity;			//!< override verbosity with command line arg
	if (logInterval>-1) gl.logInterval=logInterval; //!< override loginterval with command line arg
	if (gl.logInterval<2) gl.logInterval=2;					//!< minimum logInterval
	gl.logInterval-=1;															//!< correct log interval by a second to accound for processing

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
					bp.printInfo(gl.verbosity);
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
							bp.printInfo(gl.verbosity);
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
