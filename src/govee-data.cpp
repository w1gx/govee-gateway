#include <sys/types.h>
#include <sstream>
#include "govee-data.h"

#include <iostream>

using namespace std;

void GoveeData::decodeData(const char* const data)
{
	int temp = ((signed char)(data[4]) << 8) | int(data[3]);
  humidity = float((int(data[6]) << 8 | int(data[5])))/100;
  battery = int(data[7]);
	ma = int(data[8]);
	temperatureC = float(temp)/100;
  temperatureF = temperatureC*9/5+32;
}
