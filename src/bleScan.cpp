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

#include "bleScan.h"

BLEScan::BLEScan(void)
{
  connect();
}

bool BLEScan::connect(void)
{
  isConnected = false;

  hci_deviceID = hci_get_route(NULL);
  if (hci_deviceID < 0)
  {
    std::cerr << "Error: Bluetooth device not found." << std::endl;
    return false;
  }
  hci_deviceHandle = hci_open_dev(hci_deviceID);
  if (hci_deviceHandle < 0)
  {
    std::cerr << "Error: Cannot open device: " << strerror(errno) << std::endl;
    return false;
  }
  int on = 1;
  if (ioctl(hci_deviceHandle, FIONBIO, (char *)&on) < 0)
  {
    std::cerr << "Error: Could set device to non-blocking: " << strerror(errno) << std::endl;
    return false;
  }

  // Disable Scanning on the device before setting scan parameters
  hci_le_set_scan_enable(hci_deviceHandle, 0x00, 0x01, 1000);

  // set scan parameters
  char localName[0xff] = { 0 };
  hci_read_local_name(hci_deviceHandle, sizeof(localName), localName, 1000);
  if (hci_le_set_scan_parameters(hci_deviceHandle, 0x01, htobs(0x0012), htobs(0x0012), 0x01, 0x00, 1000) < 0)
  {
    std::cerr << "Error: Failed to set scan parameters: " << strerror(errno) << std::endl;
  } else {
    if (hci_le_set_scan_parameters(hci_deviceHandle, 0x01, htobs(0x1f40), htobs(0x1f40), 0x01, 0x00, 1000) < 0)
    {
      std::cerr << "Error: Failed to set scan parameters(Scan Interval : 8000 (5000 msec)): " << strerror(errno) << std::endl;
      return false;
    }
  }

  if (hci_le_set_scan_enable(hci_deviceHandle, 0x01, 0x00, 1000) < 0)
  {
    std::cerr << "Error: Failed to enable scan: " << strerror(errno) << std::endl;
    return false;
  }

  // Save the current HCI filter (Host Controller Interface)
  socklen_t olen = sizeof(initial_filter);
  if (0 == getsockopt(hci_deviceHandle, SOL_HCI, HCI_FILTER, &initial_filter, &olen))
  {
    // Create and set the new filter
    struct hci_filter new_filter;
    hci_filter_clear(&new_filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);
    if (setsockopt(hci_deviceHandle, SOL_HCI, HCI_FILTER, &new_filter, sizeof(new_filter)) < 0) {
      std::cerr << "Error: Could not set socket options: " << strerror(errno) << std::endl;
    } else {
      isConnected = true;
    }
  }
  return isConnected;
}

void BLEScan::disconnect(void)
{
  setsockopt(hci_deviceHandle, SOL_HCI, HCI_FILTER, &initial_filter, sizeof(initial_filter));
  hci_le_set_scan_enable(hci_deviceHandle, 0x00, 1, 1000);
  hci_close_dev(hci_deviceHandle);
}

bool BLEScan::scan(BLEPacket *packet)
{
  if (!isConnected)
  {
    std::cerr << "Error: not connected." << std::endl;
    return false;
  }

  packet->packetLength = read(hci_deviceHandle, packet->buf, sizeof(packet->buf));

  if (packet->packetLength > HCI_MAX_EVENT_SIZE)
  {
    std::cerr << "Error: Buffer (" << packet->packetLength << ") is greater than HCI_MAX_EVENT_SIZE (" << HCI_MAX_EVENT_SIZE << ")" << std::endl;
  }

  if (packet->packetLength > (HCI_EVENT_HDR_SIZE + 1 + LE_ADVERTISING_INFO_SIZE))
  {
    evt_le_meta_event *metaEvent = (evt_le_meta_event *)(packet->buf + (HCI_EVENT_HDR_SIZE + 1));
    if (metaEvent->subevent == EVT_LE_ADVERTISING_REPORT)
    {
      const le_advertising_info * const info = (le_advertising_info *)(metaEvent->data + 1);
      packet->addr[19] = { 0 };
      ba2str(&info->bdaddr, packet->addr);
      packet->bdaddr = info->bdaddr;
      packet->event_type=info->evt_type;
      packet->event_length=info->length;
      packet->subevent = metaEvent->subevent;

      // now we have the entire event in packet.
      // time to break it down into data packets.
      // each data packet consists of an octet of length, a data type and data of "length" octets
      int current_offset = 0;

      if (info->length > 0)
      {
        bool data_error = false;
        while (!data_error && current_offset < info->length)
        {
          // every response starts with octets for length(00) and type(01).
          size_t data_len = info->data[current_offset];
          if (current_offset + data_len + 1 > info->length)
          {
            std::cout << "data length is longer than packet length. " << int(data_len) << " + 1 > " << info->length << std::endl;
            data_error = true;
          } else {
            packet->adStructure_data.length = static_cast<int>(data_len);
            packet->adStructure_data.type = char(*(info->data + current_offset + 1));

            packet->adStructure_data.type = *(info->data + current_offset + 1);
            for (auto index = 0; index < char(data_len)-1; index++)
            {
              packet->adStructure_data.data[index] = char((info->data + current_offset + 2)[index]);
            }

            packet->adStructures.insert(std::make_pair(packet->adStructure_data.type, packet->adStructure_data));
          }
          current_offset += data_len+1;
        }
      }
      packet->rssi = char(info->data[current_offset]);
      return true;
    }

  } else if (errno == EAGAIN)
  {
    usleep(100);
  }
  else if (errno == EINTR)
  {
    std::cerr << "Error: " << strerror(errno) << " (" << errno << ")" << std::endl;
  }
  return false;
}
