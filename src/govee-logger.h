#ifndef __Govee_logger_H
#define __Govee_logger_H

#include <string>
#include <map>
#include <queue>

#include <iostream>
#include <iomanip>    // for setprecision

#ifdef MQTT
#include "mqtt/async_client.h"
#endif

#include "blePacket.h"
#include "govee-data.h"

//!  Operator for bdaddr_t maps
inline bool operator <(const bdaddr_t &a, const bdaddr_t &b)
{
	unsigned long long A = a.b[5];
	A = A << 8 | a.b[4];
	A = A << 8 | a.b[3];
	A = A << 8 | a.b[2];
	A = A << 8 | a.b[1];
	A = A << 8 | a.b[0];
	unsigned long long B = b.b[5];
	B = B << 8 | b.b[4];
	B = B << 8 | b.b[3];
	B = B << 8 | b.b[2];
	B = B << 8 | b.b[1];
	B = B << 8 | b.b[0];
	return(A < B);
}


class Govee_logger
{
  const int  QOS = 1;

  bool initialized=false;         //!< initialized flag
  time_t timeStart;               //!< time when interval counter starts

  // logging parameters
  int logInterval;                //!< log interval in seconds

  // mqtt parameters
  std::string mqtt_host;
  std::string mqtt_username;
  std::string mqtt_password;
  std::string mqtt_topic;
  bool mqtt_active=false;

  // influxdb parameters
  std::string influx_host;
  int influx_port;
  std::string influx_database;
  std::string influx_username;
  std::string influx_password;
  std::string influx_measurement;
  bool influx_active = false;

  // async client handle
  #ifdef MQTT
  mqtt::async_client *async_client;
  mqtt::connect_options conopts;
  mqtt::token_ptr conntok;
  #endif
  
  // dat maps
  std::map<std::string, std::string> adrMap;    //!< Address map from INI file
  std::map<bdaddr_t, std::queue<GoveeData>> govee_dataQueue; //!< data queue for govee data by address

  // private methods
  bool readConfigFile(const char* iniFileName);
  void initializeLogger(void);
  int sendData();

  public:
    int debugLevel;

    Govee_logger(const char* iniFileName);
    void logData(const BLEPacket* bp, const char* data);
};

#endif // __Govee_logger_H
