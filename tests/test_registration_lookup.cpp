// viz1090, a visualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// All rights reserved.
//
// Unit tests for RegistrationLookup

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/RegistrationLookup.h"

using namespace viz1090::core;

class RegistrationLookupTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a temporary directory for test data
    testDataDir_ = std::filesystem::temp_directory_path() / "viz1090_test_data";
    std::filesystem::create_directories(testDataDir_);

    // Create test registration files with known data
    createTestFile("A0.json", R"(
{
  "0001": {
    "r": "N123AB",
    "t": "C172",
    "f": "00",
    "desc": "Cessna 172"
  },
  "0002": {
    "r": "N456CD",
    "t": "PA28",
    "f": "00",
    "desc": "Piper Cherokee"
  },
  "8000": {
    "r": "N789EF",
    "t": "B737",
    "f": "00",
    "desc": "Boeing 737-800"
  }
}
)");

    createTestFile("3F.json", R"(
{
  "F001": {
    "r": "G-ABCD",
    "t": "A320",
    "f": "01",
    "desc": "Airbus A320"
  }
}
)");

    // Create an empty file for testing edge cases
    createTestFile("00.json", "{}");

    // Create file with malformed JSON for error handling
    createTestFile("FF.json", "{ malformed json }");
  }

  void TearDown() override {
    // Clean up test directory
    std::filesystem::remove_all(testDataDir_);
  }

  void createTestFile(const std::string& filename, const std::string& content) {
    std::ofstream file(testDataDir_ / filename);
    file << content;
    file.close();
  }

  std::filesystem::path testDataDir_;
  RegistrationLookup lookup_;
};

// Test basic initialization
TEST_F(RegistrationLookupTest, InitializeValidPath) {
  bool result = lookup_.initialize(testDataDir_.string());
  EXPECT_TRUE(result);
}

TEST_F(RegistrationLookupTest, InitializeInvalidPath) {
  bool result = lookup_.initialize("/nonexistent/path");
  EXPECT_FALSE(result);
}

TEST_F(RegistrationLookupTest, InitializeEmptyPath) {
  bool result = lookup_.initialize("");
  EXPECT_FALSE(result);
}

// Test aircraft type lookup with valid data
TEST_F(RegistrationLookupTest, LookupExistingAircraftType) {
  ASSERT_TRUE(lookup_.initialize(testDataDir_.string()));
  
  // Test lookups from A0.json
  EXPECT_EQ(lookup_.lookupAircraftType(0xA00001), "C172");
  EXPECT_EQ(lookup_.lookupAircraftType(0xA00002), "PA28");  
  EXPECT_EQ(lookup_.lookupAircraftType(0xA08000), "B737");
  
  // Test lookup from 3F.json
  EXPECT_EQ(lookup_.lookupAircraftType(0x3FF001), "A320");
}

// Test aircraft type lookup with missing data
TEST_F(RegistrationLookupTest, LookupMissingAircraftType) {
  ASSERT_TRUE(lookup_.initialize(testDataDir_.string()));
  
  // Non-existent ICAO addresses
  EXPECT_EQ(lookup_.lookupAircraftType(0xA00999), "");  // File exists but ICAO doesn't
  EXPECT_EQ(lookup_.lookupAircraftType(0xB00001), "");  // File doesn't exist
  EXPECT_EQ(lookup_.lookupAircraftType(0x000001), "");  // Empty file
}

// Test full registration data lookup
TEST_F(RegistrationLookupTest, LookupRegistrationData) {
  ASSERT_TRUE(lookup_.initialize(testDataDir_.string()));
  
  RegistrationData data;
  bool found = lookup_.lookupRegistrationData(0xA00001, data);
  
  EXPECT_TRUE(found);
  EXPECT_EQ(data.registration, "N123AB");
  EXPECT_EQ(data.type, "C172");
  EXPECT_EQ(data.flag, "00");
  EXPECT_EQ(data.description, "Cessna 172");
}

// Test lookup without initialization
TEST_F(RegistrationLookupTest, LookupWithoutInitialization) {
  // Don't call initialize
  EXPECT_EQ(lookup_.lookupAircraftType(0xA00001), "");
  
  RegistrationData data;
  EXPECT_FALSE(lookup_.lookupRegistrationData(0xA00001, data));
}

// Test caching behavior - second lookup should use cache
TEST_F(RegistrationLookupTest, CachingBehavior) {
  ASSERT_TRUE(lookup_.initialize(testDataDir_.string()));
  
  // First lookup should load file
  std::string type1 = lookup_.lookupAircraftType(0xA00001);
  EXPECT_EQ(type1, "C172");
  
  // Second lookup from same file should use cache
  std::string type2 = lookup_.lookupAircraftType(0xA00002);
  EXPECT_EQ(type2, "PA28");
  
  // Lookup from different file should load new file
  std::string type3 = lookup_.lookupAircraftType(0x3FF001);
  EXPECT_EQ(type3, "A320");
}

// Test error handling with malformed JSON
TEST_F(RegistrationLookupTest, MalformedJsonHandling) {
  ASSERT_TRUE(lookup_.initialize(testDataDir_.string()));
  
  // This should handle the malformed JSON gracefully
  std::string result = lookup_.lookupAircraftType(0xFF0001);
  EXPECT_EQ(result, "");  // Should return empty string for malformed data
}

// Test various ICAO address formats
TEST_F(RegistrationLookupTest, VariousIcaoFormats) {
  ASSERT_TRUE(lookup_.initialize(testDataDir_.string()));
  
  // Test different ways to represent the same ICAO address
  uint32_t icao1 = 0xA00001;
  uint32_t icao2 = 10485761;  // Same as 0xA00001 in decimal
  
  EXPECT_EQ(lookup_.lookupAircraftType(icao1), lookup_.lookupAircraftType(icao2));
}

// Test edge case with zero ICAO address
TEST_F(RegistrationLookupTest, ZeroIcaoAddress) {
  ASSERT_TRUE(lookup_.initialize(testDataDir_.string()));
  
  std::string result = lookup_.lookupAircraftType(0x000000);
  EXPECT_EQ(result, "");  // Should handle zero gracefully
}

// Performance test - ensure reasonable lookup times
TEST_F(RegistrationLookupTest, PerformanceTest) {
  ASSERT_TRUE(lookup_.initialize(testDataDir_.string()));
  
  // Warm up cache
  lookup_.lookupAircraftType(0xA00001);
  
  auto start = std::chrono::high_resolution_clock::now();
  
  // Perform many lookups
  for (int i = 0; i < 1000; ++i) {
    lookup_.lookupAircraftType(0xA00001);  // Cache hit
    lookup_.lookupAircraftType(0xA00002);  // Same file, different entry
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  // Should complete 2000 lookups in reasonable time (less than 100ms)
  EXPECT_LT(duration.count(), 100);
}