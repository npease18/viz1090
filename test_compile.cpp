#include "viz1090/TestData.h"
#include <iostream>

int main() {
    std::cout << "Test data contains " << viz1090::kTestAircraftData.size() << " aircraft" << std::endl;
    std::cout << "Test receiver location: " << viz1090::kTestReceiverLat << ", " << viz1090::kTestReceiverLon << std::endl;
    return 0;
}