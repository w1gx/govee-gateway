#include <map>
#include <queue>

#ifndef __GoveeData_H
#define __GoveeData_H

class  GoveeData {
public:
	float temperatureC;
	float temperatureF;
	float humidity;
	int battery;
	signed char rssi;
	void decodeData(const char* const data);
	GoveeData() : temperatureC(0), temperatureF(0), humidity(0), battery(0) { };
};

#endif //__GoveeData_H
