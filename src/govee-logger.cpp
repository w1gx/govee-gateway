#include "govee-logger.h"
#include <iostream>

#include "INIReader.h"
#include "influxdb.hpp"

#ifdef MQTT
/**
 * A callback class for use with the main MQTT client.
*/
class callback : public virtual mqtt::callback
{
public:
	void connection_lost(const std::string& cause) override {
		std::cout << "\nConnection lost" << std::endl;
		if (!cause.empty())
			std::cout << "\tcause: " << cause << std::endl;
	}

	void delivery_complete(mqtt::delivery_token_ptr tok) override {
		std::cout << "\tDelivery complete for token: "
			<< (tok ? tok->get_message_id() : -1) << std::endl;
	}
};
#endif //MQTT

//! Private methods
void Govee_logger::readConfigFile (const char* iniFileName)
{
  INIReader reader(iniFileName);
	if (reader.ParseError() < 0) {
		std::cout << "Can't load configuration file " << iniFileName << "\n";
    exit(EXIT_FAILURE);
	} else {
    logInterval = reader.GetInteger("system", "logInterval", 30);
    verbosity = reader.GetInteger("system", "verbosity", 0);
		mqtt_host = reader.Get("mqtt", "host", "NONE");
		mqtt_username = reader.Get("mqtt", "username", "");
		mqtt_password = reader.Get("mqtt", "password", "");
		mqtt_topic = reader.Get("mqtt", "topic", "govee");
    influx_host = reader.Get("influxdb", "host", "NONE");
		influx_port = reader.GetInteger("influxdb","port",8086);
		influx_username = reader.Get("influxdb", "username", "");
		influx_password = reader.Get("influxdb", "password", "");
		influx_database = reader.Get("influxdb", "database", "temp");
		influx_measurement = reader.Get("influxdb", "measurement", "govee");

    // get section keys for AddressMap section and populate goveeSerializer AddressMap
		std::map<std::string, std::queue<std::string>> sk = reader.SectionKeys();
		auto it = sk.find("AddressMap");
		if (it != sk.end())
		{
			while (!it->second.empty())
			{
				std::string entry = it->second.front();
				std::string adr = reader.Get("AddressMap",it->second.front(),"");
				std::cout << "Address mapping: " << ANSI_COLOR_RED << it->second.front() << ANSI_COLOR_RESET << " mapped to " << adr << std::endl;
				adrMap.insert(std::pair<std::string, std::string>(entry,adr));
				it->second.pop();
			}
		} else {
      std::cout << "No AddressMap section found in config file." << std::endl;
    }
  }
}

void Govee_logger::initializeLogger(void)
{
	std::cout << "Checking connections..." << std::endl;
  // initialize serialization
  #ifdef MQTT
	if (mqtt_host != "NONE" && !mqtt_host.empty())
	{
    mqtt_host = "tcp://"+mqtt_host;
    if (mqtt_username.length() > 0)
  	{
   		conopts.set_user_name(mqtt_username);
  		conopts.set_password(mqtt_password);
  	}
  	// set Will;
    mqtt::message willmsg("WILL", "LWT_PAYLOAD", 1, true);
    mqtt::will_options will(willmsg);
    conopts.set_will(will);

		std::cout << "MQTT host is '" << mqtt_host << "'...";

		// now try to connect
		mqtt::async_client client(mqtt_host, "goveeGateway");
		try {
			conntok = client.connect(conopts);
			conntok->wait();
			std::cout << ANSI_COLOR_GREEN << "ok." << ANSI_COLOR_RESET << std::endl;
			conntok = client.disconnect();
			conntok->wait();
			mqtt_active = true;
		}
		catch (const mqtt::exception& exc) {
			std::cout << ANSI_COLOR_RED << "failed. " << ANSI_COLOR_MAGENTA << exc.what() << ANSI_COLOR_RESET << std::endl;
		}
	}
  #endif

	if (influx_host != "NONE" && !influx_host.empty())
	{
		std::cout << "InfluxDB host is " << influx_host << "... ";

		// try to connect
		std::string resp;
		influxdb_cpp::server_info si(influx_host, influx_port, influx_database, influx_username, influx_password);
		influxdb_cpp::query(resp, "show databases", si);
		if (resp.length()>0)
		{
			influx_active = true;
			std::cout << ANSI_COLOR_GREEN << " ok." ANSI_COLOR_RESET << std::endl;
		} else {
			std::cout <<ANSI_COLOR_RED << " failed." << ANSI_COLOR_RESET << std::endl;
		}
	}
	if (!mqtt_active && !influx_active)
	{
		std::cout << ANSI_COLOR_RED << "No active connections available. Terminating..." << ANSI_COLOR_RESET << std::endl;
		exit(EXIT_FAILURE);
	}
	std::cout << ANSI_COLOR_YELLOW << "running..." << ANSI_COLOR_RESET << std::endl;
}

void Govee_logger::sendData()
{
	// initialize mqtt
  #ifdef MQTT
  const auto TIMEOUT = std::chrono::seconds(10);
  mqtt::async_client client(mqtt_host, "goveeLogger");
  if (mqtt_active)
  {
	  // connect
  	try {
  		conntok = client.connect(conopts);
  		conntok->wait();
  	}
  	catch (const mqtt::exception& exc) {
  		std::cerr << exc.what() << std::endl;
  	}
  }
  #endif

  // initialize influxdb
  influxdb_cpp::server_info si(influx_host, influx_port, influx_database, influx_username, influx_password);

	// create messages and send
	for (auto it = govee_dataQueue.begin(); it != govee_dataQueue.end(); ++it)
	{
		bool ismapped = false;

		if (!it->second.empty()) // Only proceed if there are entries to add
		{
			char ad[19] = { 0 };
			ba2str(&it->first, ad);
			int sz =it->second.size();
			std::string addr = ad;

			// find mapped address
			auto itadr = adrMap.find(addr);
			if (itadr != adrMap.end())
			{
				addr = itadr->second;
				ismapped = true;
			}

      if (verbosity>0)
      {
        std::cout << "Storing data for " << (ismapped?ANSI_COLOR_GREEN:ANSI_COLOR_RED) << addr << ANSI_COLOR_RESET << " (" << sz << " elements): ";
      }
			GoveeData tempData;             		//!< temporary data for average calculations
			tempData.calcAverage(&it->second);	//!< calculate average for entire queue

			// send MQTT
      #ifdef MQTT
			if (mqtt_active)
			{
				// generate message
				std::ostringstream mqtt_outStream;
				mqtt_outStream << "{";
				mqtt_outStream << "\"temp\":"  << std::dec << tempData.temperatureF << ",";
				mqtt_outStream << "\"hum\":" <<  tempData.humidity << ",";
				mqtt_outStream << "\"bat\":" << tempData.battery << ",";
				mqtt_outStream << "\"ma\":" << int(tempData.ma) << ",";
        mqtt_outStream << "\"rssi\":" << int(tempData.rssi) << "}";

				std::string mqtt_topic_complete = mqtt_topic+"/"+addr+"/DTA";

				// send message
				try {
					mqtt::message_ptr pubmsg = mqtt::make_message(mqtt_topic_complete, mqtt_outStream.str().c_str());
					pubmsg->set_qos(QOS);
					client.publish(pubmsg)->wait_for(TIMEOUT);
        	if (verbosity>0)
        	{
          	std::cout << ANSI_COLOR_GREEN << "MQTT ok. " << ANSI_COLOR_RESET;
        	}
				}
				catch (const mqtt::exception& exc) {
					std::cerr << "Error creating MQTT message. " << exc.what() << std::endl;
				}
			}
      #endif

			// send data to influxdb only if address is mapped
			if (influx_active && ismapped)
			{
				influxdb_cpp::builder()
					.meas(influx_measurement)
					.tag("ADR",addr)
					.field("temp",tempData.temperatureF)
					.field("hum",tempData.humidity)
					.field("bat",tempData.battery)
          .field("rssi",tempData.rssi)
					.field("ma",tempData.ma)
					.post_http(si);
        if (verbosity>0)
        {
          std::cout << ANSI_COLOR_GREEN << "InfluxDB ok." << ANSI_COLOR_RESET ;
        }
			} else if (influx_active && verbosity>0) {
				std::cout << ANSI_COLOR_YELLOW << "skipping InfluxDB " ;
        if (!ismapped)
        {
          std::cout << ANSI_COLOR_MAGENTA << "(device not mapped, please check configuration).. " << ANSI_COLOR_RESET;
        }
			}
      if (verbosity>0)
      {
        std::cout << "." << std::endl;
      }
		}
	}

	// disconnect from MQTT
  #ifdef MQTT
	if (mqtt_active)
	{
		try {
			conntok = client.disconnect();
			conntok->wait();
		}
		catch (const mqtt::exception& exc) {
			std::cerr << exc.what() << std::endl;
		}
	}
  #endif
}

//! Constructor
Govee_logger::Govee_logger(const char* iniFileName)
{
  readConfigFile(iniFileName);
  initializeLogger();
  time(&timeStart);
  initialized=true;
}

void Govee_logger::logData(const BLEPacket *bp, const char* data)
{
  if (initialized)    // check if the logger has been intialized
  {
    // check if we have govee data
    if (data[0]==0x88 && data[1]==0xEC)
    {
      GoveeData gd;
      gd.decodeData(data);
      gd.rssi = bp->rssi;

      // log data into queue
      std::queue<GoveeData> foo;
      auto ret = govee_dataQueue.insert(std::pair<bdaddr_t, std::queue<GoveeData>>(bp->bdaddr, foo));
      ret.first->second.push(gd);

      // print data
      if (verbosity>0)
      {
        std::cout << "Data logged. Queue#: " << std::setw(2) << ret.first->second.size() << ". ";
        std::cout.setf(std::ios::fixed);
        std::cout << std::setprecision(2);
        std::cout << ANSI_BOLD << ANSI_COLOR_BLUE << "GOVEE " << ANSI_COLOR_RED << bp->addr << ANSI_COLOR_RESET;
        std::cout << " - Temp=" << gd.temperatureC << "C (" << gd.temperatureF <<"F), Hum="<< gd.humidity;
        std::cout << "%, Bat=" << gd.battery << "%, RSSI= "<< int(gd.rssi) << "dBm, MA=" << int(gd.ma) << std::endl;
      }

      // check interval and send data
      time_t timeNow;
			time(&timeNow);
			if (difftime(timeNow, timeStart) > logInterval)
			{
        sendData();
        timeStart=timeNow;
      }
    } // govee data
  } // initialized
}
