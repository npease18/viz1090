#include "Battery.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdio>

namespace viz1090 {
namespace core {

Battery::Battery() : batteryPath_("/sys/class/power_supply/BAT0") {
  // Initialize status as invalid
  status_.isValid = false;
  status_.isPresent = false;
  
  // Check if battery path exists
  if (std::filesystem::exists(batteryPath_)) {
    status_.isPresent = true;
  }
}

std::string Battery::readBatteryFile(const std::string& filename) const {
  if (!status_.isPresent) {
    return "";
  }
  
  std::string filepath = batteryPath_ + "/" + filename;
  std::ifstream file(filepath);
  
  if (!file.is_open()) {
    return "";
  }
  
  std::string content;
  std::getline(file, content);
  
  // Remove any trailing whitespace/newlines
  content.erase(content.find_last_not_of(" \t\n\r\f\v") + 1);
  
  return content;
}

Battery::Status Battery::readStatus() {
  if (!status_.isPresent) {
    return status_;
  }
  
  try {
    // Read battery capacity (percentage)
    std::string capacityStr = readBatteryFile("capacity");
    if (capacityStr.empty()) {
      status_.isValid = false;
      return status_;
    }
    
    status_.percentage = std::stoi(capacityStr);
    status_.percentage = std::max(0, std::min(100, status_.percentage));
    
    // Read charging status
    std::string statusStr = readBatteryFile("status");
    status_.isCharging = (statusStr == "Charging");
    
    status_.isValid = true;
    
  } catch (const std::exception&) {
    status_.isValid = false;
    status_.percentage = 0;
    status_.isCharging = false;
  }
  
  return status_;
}

}  // namespace core
}  // namespace viz1090