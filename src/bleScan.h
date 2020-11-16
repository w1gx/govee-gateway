#ifndef __BLE_SCAN_H
#define __BLE_SCAN_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <queue>

#include "blePacket.h"

//!  BLE Scanner class
/*!
  Contains functionality to connect to BLE, disconnect from BLE and scan all advertising packets
*/
class BLEScan {
    struct hci_filter initial_filter;   //!< storage for the intial filter
    bool isConnected;                   //!< flag to show if hci is connected
    int hci_deviceID;                   //!< hci device id
    int hci_deviceHandle;               //!< hci device handle

public:
    BLEScan(void);                      //!< Constructor
    bool connect(void);                 //!< connect to BLE
    bool scan (BLEPacket *packet);      //!< scan BLE packets
    void disconnect(void);              //!< disconnect from BLE
};




#endif //__BLE_SCAN_H
