#include "core/RegistrationLookup.h"
#include <iostream>

int main() {
    viz1090::core::RegistrationLookup lookup;
    
    if (!lookup.initialize("registration_data")) {
        std::cerr << "Failed to initialize" << std::endl;
        return 1;
    }
    
    // Test some known addresses from A0.json
    // From the file we saw: "E000":{"r":"N1556V","t":"C172","f":"00","desc":"R172K"}
    // This should be ICAO A0E000 (hex)
    uint32_t testAddr = 0xA0E000;
    std::string type = lookup.lookupAircraftType(testAddr);
    
    std::cout << "ICAO: " << std::hex << testAddr;
    std::cout << " -> Type: '" << type << "'" << std::endl;
    
    // Test the conversion functions directly
    std::string hex = lookup.icaoToHex(testAddr);
    std::string prefix = lookup.getFilePrefix(hex);
    std::cout << "Hex: " << hex << ", Prefix: " << prefix << std::endl;
    
    return 0;
}