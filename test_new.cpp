#include <iostream>
#include "core/RegistrationLookup.h"
int main() { viz1090::core::RegistrationLookup lookup; lookup.initialize("registration_data"); std::cout << "A12345 -> " << lookup.lookupAircraftType(0xA12345) << std::endl; return 0; }
