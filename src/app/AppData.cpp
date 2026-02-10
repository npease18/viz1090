// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// Copyright (C) 2014, Malcolm Robb <Support@ATTAvionics.com>
// Copyright (C) 2012, Salvatore Sanfilippo <antirez at gmail dot com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "app/AppData.h"

#include <cstdio>
#include "viz1090/TestData.h"

AppData::AppData()
    : mConnectionManager(std::make_unique<viz1090::network::ConnectionManager>()),
      mLastCleanup(std::chrono::steady_clock::now()) {
  // Set up message handler
  mConnectionManager->onMessage(
      [this](const viz1090::ModesMessage& aMsg) { handleMessage(aMsg); });

  // Set up state change handler
  mConnectionManager->onStateChange(
      [](bool aConnected, const std::string& aHost) {
        if (aConnected) {
          std::fprintf(stderr, "Connected to %s\n", aHost.c_str());
        } else {
          std::fprintf(stderr, "Disconnected from %s\n", aHost.c_str());
        }
      });
}

AppData::~AppData() {
  disconnect();
}

void
AppData::initialize() {
  // No initialization needed with new architecture
}

void
AppData::connect() {
  if (mConnectionManager->isRunning()) {
    return;
  }

  std::fprintf(stderr, "Connecting to %s:%d\n", server.c_str(), port);

  viz1090::network::ConnectionManager::Config config;
  config.host = server;
  config.port = port;
  config.autoReconnect = true;
  config.reconnectDelay = std::chrono::seconds{5};

  mConnectionManager->start(config);
}

void
AppData::disconnect() {
  mConnectionManager->stop();
}

bool
AppData::isConnected() const {
  // In test mode, always report as connected
  if (mTestMode) {
    return true;
  }
  return mConnectionManager->isConnected();
}

void
AppData::update() {
  // Remove stale aircraft periodically (skip in test mode)
  auto now = std::chrono::steady_clock::now();
  if (!mTestMode && now - mLastCleanup > kCleanupInterval) {
    removeStaleAircraft();
    mLastCleanup = now;
  }

  // Update statistics
  updateStatus();
  
  // Update battery status
  mBattery.readStatus();
}

void
AppData::handleMessage(const viz1090::ModesMessage& aMsg) {
  std::lock_guard<std::mutex> lock(mMessageMutex);
  aircraftList.updateFromMessage(aMsg);
}

void
AppData::removeStaleAircraft() {
  std::lock_guard<std::mutex> lock(mMessageMutex);
  aircraftList.removeStale(kAircraftTtl);
}

void
AppData::updateStatus() {
  numVisiblePlanes = 0;
  numPlanes = 0;
  double sigAccumulate = 0.0;
  double msgRateAccumulate = 0.0;

  std::lock_guard<std::mutex> lock(mMessageMutex);

  for (const auto& p : aircraftList) {
    unsigned char* pSig = p->signalLevel;
    unsigned int signalAverage =
        (pSig[0] + pSig[1] + pSig[2] + pSig[3] + pSig[4] + pSig[5] + pSig[6] +
         pSig[7]);

    sigAccumulate += signalAverage;

    if (p->lon != 0.0f && p->lat != 0.0f) {
      numVisiblePlanes++;
    }

    msgRateAccumulate += p->messageRate;

    numPlanes++;
  }

  msgRate = msgRateAccumulate;
  if (numPlanes > 0) {
    avgSig = sigAccumulate / static_cast<double>(numPlanes);
  } else {
    avgSig = 0.0;
  }
}

void
AppData::loadTestData() {
  std::lock_guard<std::mutex> lock(mMessageMutex);
  
  // Enable test mode
  mTestMode = true;
  
  // Clear any existing aircraft by removing all stale aircraft with 0 TTL
  aircraftList.removeStale(std::chrono::seconds{0});
  
  // Set receiver position to test location
  userLat = viz1090::kTestReceiverLat;
  userLon = viz1090::kTestReceiverLon;
  
  // Create aircraft from test data
  auto currentTime = std::chrono::high_resolution_clock::now();
  
  for (const auto& testData : viz1090::kTestAircraftData) {
    auto* aircraft = aircraftList.findOrCreate(testData.icao);
    
    // Set basic identity
    std::strncpy(aircraft->flight, testData.callsign, sizeof(aircraft->flight) - 1);
    aircraft->flight[sizeof(aircraft->flight) - 1] = '\0';
    
    // Set position
    aircraft->lat = static_cast<float>(testData.latitude);
    aircraft->lon = static_cast<float>(testData.longitude);
    
    // Set flight parameters
    aircraft->altitude = testData.altitude;
    aircraft->speed = testData.speed;
    aircraft->track = testData.heading;
    aircraft->vert_rate = testData.verticalRate;
    
    // Set timing
    aircraft->seen = std::time(nullptr);
    aircraft->seenLatLon = aircraft->seen;
    aircraft->msSeen = currentTime;
    aircraft->msSeenLatLon = currentTime;
    aircraft->created = currentTime;
    
    // Set signal levels (simulate good reception)
    for (int i = 0; i < 8; i++) {
      aircraft->signalLevel[i] = 180 + (testData.icao % 40); // Vary signal by ICAO
    }
    
    // Set message rate (simulate active aircraft)
    aircraft->messageRate = 5.0f + static_cast<float>(testData.icao % 20) / 10.0f;
    
    // Mark as active
    aircraft->live = 1;
    
    // Debug output
    std::fprintf(stderr, "Test aircraft %s: lat=%.4f, lon=%.4f, alt=%d\n", 
                 aircraft->flight, aircraft->lat, aircraft->lon, aircraft->altitude);
  }
  
  std::fprintf(stderr, "Loaded %zu test aircraft\n", viz1090::kTestAircraftData.size());
}
