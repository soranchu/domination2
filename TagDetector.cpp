#include "mbed.h"
#include "ble/BLE.h"
#include "TagDetector.h"

const uint8_t TagDetector::TAG_SIZE;
const uint8_t TagDetector::TEAM_RED;
const uint8_t TagDetector::TEAM_YELLOW;
const uint8_t TagDetector::SERVICES[6] = {0x03, 0x18, 0x02, 0x18, 0x04, 0x18};
const TagDetector::Tag_t TagDetector::TAGS[] = {
  {{0xfc, 0x58, 0xfa, 0x19, 0xdf, 0x7f}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xbc, 0x81}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xbe, 0x3b}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xbe, 0x18}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xc6, 0x22}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xc0, 0x8b}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xbc, 0xc8}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xcc, 0x27}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xbd, 0xc8}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xbe, 0x34}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xc5, 0xef}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xce, 0xb1}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xdf, 0x78}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xc6, 0x09}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xc2, 0x84}, 'R'},  // R
  {{0xfc, 0x58, 0xfa, 0x19, 0xe6, 0xfb}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe7, 0x93}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0x77, 0x4f}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xdf, 0xc5}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe5, 0x99}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe8, 0x34}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe7, 0xcb}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe5, 0x4a}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe4, 0x8c}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe6, 0xdb}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0x77, 0x82}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0x68, 0x8a}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe6, 0xcf}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe5, 0x47}, 'Y'},  // Y
  {{0xfc, 0x58, 0xfa, 0x19, 0xe5, 0x77}, 'Y'}   // Y
};

TagDetector::TagDetector() {
  teams[0].type ='R';
  teams[1].type ='Y';
  nodeState = 0xff;
  senseLevel = -66;
} 

void TagDetector::scanCallback(const Gap::AdvertisementCallbackParams_t* params) {
  // parse the advertising payload, looking for data type COMPLETE_LOCAL_NAME
  // The advertising payload is a collection of key/value records where
  // byte 0: length of the record excluding this byte
  // byte 1: The key, it is the type of the data
  // byte [2..N] The value. N is equal to byte0 - 1

  for (uint8_t i = 0; i < params->advertisingDataLen; ++i) {
    const uint8_t record_length = params->advertisingData[i];
    if (record_length == 0) {
      continue;
    }
    const uint8_t type = params->advertisingData[i + 1];
    const uint8_t* value = params->advertisingData + i + 2;
    const uint8_t value_length = record_length - 1;

    i += record_length;
    if (value_length == 6 && memcmp(value, SERVICES, 6) == 0 && params->rssi > senseLevel) {
      Peripheral_t peri(params->peerAddr);
      std::pair<std::set<Peripheral_t>::iterator, bool> ret =
          devices.insert(peri);
      if (ret.second) {
      }
      int8_t idx = findTagId(ret.first->address);
      if (idx >= 0) {
        statuses[idx].visibleCount = 10;
      }
      break;
    }
  }
}

void TagDetector::tick() {
  uint8_t counts[2] = {0};

  for (uint8_t i = 0; i < TAG_SIZE; ++i) {
    TagStatus_t& s = statuses[i];
    if (s.visibleCount > 0) {
      s.visibleCount--;
    }
    if (s.visibleCount > 0) {
      counts[tagIdxToTeam(i)]++;
    }
  }
  teams[TEAM_RED].tagCount = counts[TEAM_RED];
  teams[TEAM_YELLOW].tagCount = counts[TEAM_YELLOW];

  if (teams[TEAM_RED].tagCount == teams[TEAM_YELLOW].tagCount) {
    nodeState = 0xff;
  } else if (teams[TEAM_RED].tagCount > teams[TEAM_YELLOW].tagCount) {
    nodeState = 0;
  } else {
    nodeState = 1;
  }
}