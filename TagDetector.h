#ifndef _TAG_DETECTOR_H_
#define _TAG_DETECTOR_H_

#include <inttypes.h>
#include "ble/BLE.h"
#include <set>

class TagDetector {
public:
  TagDetector();
  void scanCallback(const Gap::AdvertisementCallbackParams_t* params);
  void tick();
  uint8_t getState() {
    return nodeState; 
  }
  uint8_t getCount(uint8_t team) {
    return teams[team].tagCount;
  }
  struct Peripheral_t {
    BLEProtocol::AddressBytes_t address; /**< The BLE address. */

    /**
    * Construct an Address_t object with the supplied type and address.
    *
    * @param[in] typeIn
    *              The BLE address type.
    * @param[in] addressIn
    *              The BLE address.
    */
    Peripheral_t(const BLEProtocol::AddressBytes_t addressIn) {
      std::copy(addressIn, addressIn + BLEProtocol::ADDR_LEN, address);
    }

    /**
    * Empty constructor.
    */
    Peripheral_t() : address() {}
  };
  static const uint8_t TEAM_RED = 0;
  static const uint8_t TEAM_YELLOW = 1;

private:
  struct Tag_t {
    const BLEProtocol::AddressBytes_t address; /**< The BLE address. */
    const char type;
  };
  struct TagStatus_t {
    volatile bool visible;
    volatile uint8_t visibleCount;
  };
  struct Team_t {
    uint8_t tagCount;
    char type;
  };

  static const uint8_t TAG_SIZE = 30;
  static const Tag_t TAGS[TAG_SIZE];
  static const uint8_t SERVICES[6];

  Team_t teams[2];
  int8_t nodeState;
  TagStatus_t statuses[TAG_SIZE];
  std::set<Peripheral_t> devices;

  bool tagIdxToTeam(uint8_t idx) {
    return idx < 15 ? TEAM_RED : TEAM_YELLOW;
  }
  int8_t findTagId(const BLEProtocol::AddressBytes_t address) {
    for (uint8_t i = 0; i < TAG_SIZE; ++i) {
      if (memcmp(address, TAGS[i].address, 6) == 0) {
        return i;
      }
    }
    return -1;
  }
};

inline bool operator<(const TagDetector::Peripheral_t& t1, const TagDetector::Peripheral_t& t2) {
  return memcmp(t1.address, t2.address, BLEProtocol::ADDR_LEN) < 0;
}
inline bool operator>(const TagDetector::Peripheral_t& t1, const TagDetector::Peripheral_t& t2) {
  return t2 < t1;
}
inline bool operator<=(const TagDetector::Peripheral_t& t1, const TagDetector::Peripheral_t& t2) {
  return !(t1 > t2);
}
inline bool operator>=(const TagDetector::Peripheral_t& t1, const TagDetector::Peripheral_t& t2) {
  return !(t1 < t2);
}
inline bool operator==(const TagDetector::Peripheral_t& t1, const TagDetector::Peripheral_t& t2) {
  return !(t1 < t2) && !(t2 > t2);
}  // ...(*1)
inline bool operator!=(const TagDetector::Peripheral_t& t1, const TagDetector::Peripheral_t& t2) {
  return !(t1 == t2);
}

#endif