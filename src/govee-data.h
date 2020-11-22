#include <map>
#include <queue>

#ifndef __GoveeData_H
#define __GoveeData_H

class  GoveeData {
public:
	float temperatureC;			//!< temperature in C
	float temperatureF;			//!< temperature in F
	float humidity;					//!< humidity in %
	int battery;						//!< battery level in %
	signed char rssi;				//!< rssi in dBm
	signed char ma;					//!< mysterious attribute

	// decode data from info packet
	void decodeData(const char* const data);
	// constructor
	GoveeData() : temperatureC(0), temperatureF(0), humidity(0), battery(0), rssi(0), ma(0){ };
};

#endif //__GoveeData_H
