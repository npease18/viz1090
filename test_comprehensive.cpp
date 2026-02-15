#include "core/RegistrationLookup.h"
#include <iostream>
#include <vector>

int main() {
    viz1090::core::RegistrationLookup lookup;
    
    if (!lookup.initialize("registration_data")) {
        std::cerr << "Failed to initialize" << std::endl;
        return 1;
    }
    
    std::cout << "=== Registration Lookup Comprehensive Test ===" << std::endl;
    
    // Test cases based on actual data from A0.json
    std::vector<std::pair<uint32_t, std::string>> testCases = {
        {0xA0E000, "C172"},  // "E000" in A0.json
        {0xA0E001, "AT5T"},  // "E001" in A0.json
        {0xA0E002, "PA32"},  // "E002" in A0.json
        {0xA0E037, "C172"},  // "E037" in A0.json
        {0xA15000, ""},      // Should be in A1 file (might not exist)
        {0x3FF000, ""},      // Should be in 3F file
        {0x123456, ""},      // Random address that probably doesn't exist
    };
    
    std::cout << "\nTesting specific ICAO addresses:" << std::endl;
    for (const auto& testCase : testCases) {
        uint32_t icao = testCase.first;
        std::string expectedType = testCase.second;
        
        std::string actualType = lookup.lookupAircraftType(icao);
        
        std::cout << "ICAO: 0x" << std::hex << icao 
                  << " -> Found: '" << actualType << "'";
        if (!expectedType.empty()) {
            std::cout << " (Expected: '" << expectedType << "')";
            if (actualType == expectedType) {
                std::cout << " ✓";
            } else {
                std::cout << " ✗";
            }
        }
        std::cout << std::endl;
    }
    
    // Test conversion functions directly
    std::cout << "\n=== Testing Conversion Functions ===" << std::endl;
    std::vector<uint32_t> addresses = {0xA0E000, 0xA15234, 0x3FF123, 0x123456};
    
    for (uint32_t addr : addresses) {
        std::string hex = lookup.icaoToHex(addr);
        std::string prefix = lookup.getFilePrefix(hex);
        std::cout << "0x" << std::hex << addr 
                  << " -> hex: " << hex 
                  << " -> prefix: " << prefix << std::endl;
    }
    
    // Test some edge cases
    std::cout << "\n=== Testing Edge Cases ===" << std::endl;
    std::vector<uint32_t> edgeCases = {0x000000, 0xFFFFFF, 0x000001, 0xFFFFFE};
    
    for (uint32_t addr : edgeCases) {
        std::string type = lookup.lookupAircraftType(addr);
        std::cout << "Edge case 0x" << std::hex << addr 
                  << " -> '" << type << "'" << std::endl;
    }
    
    return 0;
}