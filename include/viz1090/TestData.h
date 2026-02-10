// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// All rights reserved.
//
// Test data for demonstration mode

#ifndef VIZ1090_TESTDATA_H
#define VIZ1090_TESTDATA_H

#include <array>
#include <chrono>
#include "Types.h"

namespace viz1090 {

/// Sample aircraft data for test mode
struct TestAircraftData {
  IcaoAddress icao;
  const char* callsign;
  double latitude;
  double longitude;  
  int altitude;        // feet
  int speed;          // knots
  int heading;        // degrees
  int verticalRate;   // fpm
  SquawkCode squawk;
};

/// Sample test aircraft around San Francisco Bay Area (37.7749, -122.4194)
/// This provides a realistic demonstration with various aircraft types and flight patterns
inline constexpr std::array<TestAircraftData, 15> kTestAircraftData = {{
  // Commercial airliners
  {0xA12345, "UAL123", 37.6213, -122.3790, 2800, 140, 95, -800, 1200},  // United approach to SFO
  {0xA23456, "SWA456", 37.8044, -122.2711, 35000, 480, 270, 0, 2000},   // Southwest cruise
  {0xA34567, "AAL789", 37.9838, -122.5311, 15000, 320, 180, 1200, 1000}, // American climbing
  {0xA45678, "DAL321", 37.5407, -122.0547, 41000, 520, 60, 0, 1200},   // Delta at cruise altitude
  {0xA56789, "JBU654", 37.4419, -122.1430, 8500, 220, 315, -600, 2100}, // JetBlue descending
  
  // Regional/smaller aircraft
  {0xB12345, "SKW987", 37.3639, -122.0161, 5000, 180, 45, 500, 1000},  // SkyWest regional
  {0xB23456, "ASA234", 37.8532, -122.3058, 12500, 280, 135, 0, 4000},   // Alaska Airlines
  {0xB34567, "UAL567", 37.5165, -121.9299, 1200, 90, 270, -300, 1000},  // United Express pattern
  
  // Private aircraft
  {0xC12345, "N123AB", 37.6891, -122.5964, 3500, 120, 180, 200, 1200},  // Private jet
  {0xC23456, "N456CD", 37.2431, -121.9110, 1500, 85, 90, 0, 1000},     // Small plane
  {0xC34567, "N789EF", 37.9161, -122.3181, 800, 60, 225, -200, 1200},   // Traffic pattern
  
  // Helicopters  
  {0xD12345, "CHP123", 37.7849, -122.4094, 1000, 80, 360, 0, 1200},     // Police helicopter
  {0xD23456, "LIFE45", 37.6624, -122.4836, 500, 45, 135, 100, 1200},    // Medical helicopter
  
  // Cargo/freight
  {0xE12345, "FDX890", 37.4812, -122.4364, 25000, 350, 315, 0, 5000},   // FedEx
  {0xE23456, "UPS123", 38.0531, -122.2497, 18000, 300, 225, -400, 5000} // UPS
}};

/// Test receiver location (San Francisco International Airport)
inline constexpr double kTestReceiverLat = 37.6213;
inline constexpr double kTestReceiverLon = -122.3790;

}  // namespace viz1090

#endif  // VIZ1090_TESTDATA_H