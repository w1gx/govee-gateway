#ifndef __BLE_PACKET_H
#define __BLE_PACKET_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define ANSI_BOLD          "\x1b[1m"  /**< Console color definiton */
#define ANSI_COLOR_RED     "\x1b[31m" /**< Console color definiton */
#define ANSI_COLOR_GREEN   "\x1b[32m" /**< Console color definiton */
#define ANSI_COLOR_YELLOW  "\x1b[33m" /**< Console color definiton */
#define ANSI_COLOR_BLUE    "\x1b[34m" /**< Console color definiton */
#define ANSI_COLOR_MAGENTA "\x1b[35m" /**< Console color definiton */
#define ANSI_COLOR_CYAN    "\x1b[36m" /**< Console color definiton */
#define ANSI_COLOR_RESET   "\x1b[0m"  /**< Console color definiton */

//!  BLE Packet class
/*!
Stores all packet data and contains functionality to output packet information
*/
class BLEPacket {

public:
  //! info packet structure
  typedef struct {
    char	length;
    char	type;
    char  data[50];
  } t_adStructure;

  bdaddr_t bdaddr;
  char addr[20] = { 0 };  //!< address
  unsigned char buf[HCI_MAX_EVENT_SIZE];  //!< packet buffer
  ssize_t packetLength; //!< packet length
  char subevent;        //!< sub event
  char event_type;      //!< event type
  char addr_type;       //!< address type
  char event_length;    //!< event length (for all contained info packets)
  char rssi;            //!< RSSI in dBM

  t_adStructure adStructure_data;             //!< AD structure packet
  std::map<int,t_adStructure> adStructures;   //!< map for all AD structures of packet

  void printInfo(int debugLevel);  //!< return packet info with debug level of 0-3 

};

#endif //__BLE_PACKET_H
