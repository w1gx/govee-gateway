#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <queue>
#include <sys/ioctl.h>
#include <sstream>
#include <sys/types.h>
#include <iomanip>
#include <unistd.h>

#include "blePacket.h"

void BLEPacket::printInfo(int debugLevel)
{
  std::stringstream co;
  signed char r = rssi;
  if (debugLevel>1)
  {
    co  << "PACKET from " << ANSI_COLOR_RED << addr << ANSI_COLOR_RESET ;
    co << " (" << std::dec << packetLength << " bytes," << ANSI_COLOR_YELLOW ;
    co << " RSSI= "  << std::dec << int(r) << "dBm):" << ANSI_COLOR_RESET << std::endl;
    for (auto index = 0; index < ((packetLength < 50) ? packetLength : 50); index++)
    {
      if (index == int(packetLength)-1) // RSSI
      {
        co << ANSI_COLOR_YELLOW;
      } else if (index > 6 && index < 13) // Address
      {
        co << ANSI_COLOR_RED;
      } else if (index > 13 && index < packetLength-1) // AD Info block
      {
        co << ANSI_COLOR_GREEN;
      }
      co << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << int(buf[index]) << " ";
      co << ANSI_COLOR_RESET;
    }
    co << std::endl;
    if (debugLevel>2)
    {
      co << "^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^                ^^--> event length (bytes following) = " << std::setw(2) << std::setfill('0') << std::dec << int(event_length) << std::endl;
      co << "|  |  |  |  |  |  |  ^---------------------> address: " << ANSI_COLOR_RED << addr << ANSI_COLOR_RESET << std::endl;
      co << "|  |  |  |  |  |  ^------------------------> address type: " << std::setw(2) << std::setfill('0') << int(buf[6]) << "[";
      switch (buf[6]) {
        case 0x00:
        co << "PUBLIC DEVICE ADDR";
        break;
        case 0x01:
        co << "RANDOM DEVICE ADDR";
        break;
        case 0x02:
        co << "PUBLIC ID ADDR";
        break;
        case 0x03:
        co << "RANDOM ID ADDR";
        break;
        default:
        co << "OTHER";
        break;
      }
      co << "]"<< std::endl;

      co << "|  |  |  |  |  ^---------------------------> event type: " << std::setw(2) << std::setfill('0') << int(buf[5]) << "[";
      switch (event_type) {
        case 0x00:
        co << "ADV_IND";
        break;
        case 0x01:
        co << "ADV_DIRECT_IND";
        break;
        case 0x02:
        co << "ADV_SCAN_IND";
        break;
        case 0x03:
        co << "ADV_NONCONN_IND";
        break;
        case 0x04:
        co << "ADV_SCAN_RSP";
        break;
        default:
        co << "ADV OTHER";
        break;
      }
      co << "]"<< std::endl;

      co << "|  |  |  |  ^------------------------------> number of responses: " << std::setw(2) << std::setfill('0') << int(buf[4]) << std::endl;
      co << "|  |  |  ^---------------------------------> Subevent: " << std::setw(2) << std::setfill('0') << int(buf[3]) << "[";
      switch (subevent) {
        case 0x01:
        co << "EVT_LE_CONNECTION COMPLETE";
        break;
        case 0x02:
        co << "EVT_LE_ADVERTISING";
        break;
        case 0x03:
        co << "EVT_LE_CONNECTION UPDATE";
        break;
        default:
        co << "EVT_LE_OTHER";
        break;
      }
      co << "]"<< std::endl;
      co << "|  |  ^------------------------------------> Bytes following: " << std::setw(2) << std::setfill('0') << std::dec << int(buf[2]) << std::endl;
      co << "|  ^---------------------------------------> Event Code: " << std::setw(2) << std::setfill('0') << int(buf[1]) << "[EVT_LE_META_EVENT]" << std::endl;
      co << "^------------------------------------------> Packet indicator: " << std::setw(2) << std::setfill('0') << int(buf[0]) << "[HCI_EVENT_PKT]" << std::endl;
    }

    if (debugLevel > 1)
    {
      if (event_length > 1)
      {
        if (debugLevel > 2)
        {
          co << "AD structure data: " << ANSI_COLOR_GREEN;
          for (auto index = 0 ; index <  event_length; index++)
          {
            co << " " << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << int(buf[index+14]);
          }
          co << ANSI_COLOR_RESET << std::endl;
          co << "AD structure blocks: " << std::endl;
        }


        // cycle through blocks
        std::map<int,t_adStructure>::iterator it = adStructures.begin();
        while (it!=adStructures.end())
        {
          int infotype = it->first;
          co <<  std::dec << std::setw(2) << std::setfill(' ') << int(it->second.length) <<" octets: " << std::hex << std::setw(2) << std::setfill('0') << infotype << " - ";
          co << ANSI_COLOR_MAGENTA;
          switch (infotype)
          {
            case 0x01: // Flags
            co << "Flags";
            break;
            case 0x03: // 16 bit UUIDs
            co << "UUIDs (16 bit)";
            break;
            case 0x05: // 32 bit UUIDs
            co << "UUIDs (32 bit)";
            break;
            case 0x07: // 128 bit UUIDs
            co << "UUID (128 bit)";
            break;
            case 0x09: // complete local name
            co << "Name: " << ANSI_COLOR_BLUE;
            for (auto index = 0; index < int(it->second.length)-1; index++)
            {
              co << std::uppercase << char(it->second.data[index]);
            }
            break;
            case 0x0A: // Tx Power Level
            co << "TX Power Level";
            break;
            case 0x16: // Service data
            co << "Service data";
            break;
            case 0xFF: // Manufacturer
            co << "Manufacturer data";
            break;

            default:
            co << "Other";
            break;
          }

          co << ANSI_COLOR_RESET << " [ " << ANSI_COLOR_CYAN;
          for (auto index = 0; index < int(it->second.length)-1; index++)
          {
            co << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << int(it->second.data[index])<<" ";
          }
          co << ANSI_COLOR_RESET << "] " ;


          co << ANSI_COLOR_RESET << std::endl;
          it++;
        }
      } // event_length > 0
    }
    std::cout << co.str();
  }
}
