#include "Battery.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <numeric>

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
    
    int newPercentage = std::stoi(capacityStr);
    newPercentage = std::max(0, std::min(100, newPercentage));
    
    // Read charging status
    std::string statusStr = readBatteryFile("status");
    bool newCharging = (statusStr == "Charging");
    
    // Update history and calculate runtime estimate
    updateHistory(newPercentage);
    calculateRuntimeEstimate();
    
    // Update status
    status_.percentage = newPercentage;
    status_.isCharging = newCharging;
    status_.isValid = true;
    
  } catch (const std::exception&) {
    status_.isValid = false;
    status_.percentage = 0;
    status_.isCharging = false;
    status_.remainingMinutes = -1;
    status_.hasRuntimeEstimate = false;
  }
  
  return status_;
}

void Battery::updateHistory(int percentage) {
  auto now = std::chrono::steady_clock::now();
  
  // Add new reading
  history_.push_back({percentage, now});
  
  // Remove old readings beyond max size
  if (history_.size() > MAX_HISTORY_SIZE) {
    history_.erase(history_.begin());
  }
  
  // Remove readings older than needed for calculation
  auto cutoff = now - std::chrono::minutes(30); // Keep 30 minutes of history
  history_.erase(
    std::remove_if(history_.begin(), history_.end(),
      [cutoff](const BatteryReading& reading) {
        return reading.timestamp < cutoff;
      }),
    history_.end());
}

double Battery::getDischargeRate() const {
  if (history_.size() < 2) {
    return 0.0;
  }
  
  // Calculate average discharge rate using linear regression approach
  double sumTime = 0.0;
  double sumPercentage = 0.0; 
  double sumTimePercentage = 0.0;
  double sumTimeSquared = 0.0;
  int n = static_cast<int>(history_.size());
  
  auto baseTime = history_[0].timestamp;
  
  for (const auto& reading : history_) {
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(
      reading.timestamp - baseTime).count();
    double time = static_cast<double>(minutes);
    double percentage = static_cast<double>(reading.percentage);
    
    sumTime += time;
    sumPercentage += percentage;
    sumTimePercentage += time * percentage;
    sumTimeSquared += time * time;
  }
  
  // Calculate slope (percentage change per minute)
  double denominator = n * sumTimeSquared - sumTime * sumTime;
  if (std::abs(denominator) < 1e-6) {
    return 0.0;
  }
  
  double slope = (n * sumTimePercentage - sumTime * sumPercentage) / denominator;
  return slope; // Positive = charging, Negative = discharging
}

void Battery::calculateRuntimeEstimate() {
  status_.hasRuntimeEstimate = false;
  status_.remainingMinutes = -1;
  
  if (history_.size() < 2) {
    // Provide initial rough estimate with single data point
    provideInitialEstimate();
    return;
  }
  
  // Check if we have enough data for improved estimate
  auto oldest = history_.front().timestamp;
  auto newest = history_.back().timestamp;
  auto dataSpan = std::chrono::duration_cast<std::chrono::minutes>(newest - oldest);
  
  if (dataSpan < MIN_HISTORY_TIME) {
    // Use simple estimate with available data
    provideSimpleEstimate();
    return;
  }
  
  double rate = getDischargeRate();
  
  if (status_.isCharging) {
    // When charging, estimate time to full (100%)
    if (rate > 0.01) { // Positive rate = charging
      int remainingPercent = 100 - status_.percentage;
      status_.remainingMinutes = static_cast<int>(remainingPercent / rate);
      status_.hasRuntimeEstimate = true;
    }
  } else {
    // When discharging, estimate time until empty (0%)
    if (rate < -0.01) { // Negative rate = discharging
      status_.remainingMinutes = static_cast<int>(status_.percentage / (-rate));
      status_.hasRuntimeEstimate = true;
    }
  }
  
  // Clamp runtime to reasonable bounds (0-24 hours)
  if (status_.hasRuntimeEstimate) {
    status_.remainingMinutes = std::max(0, std::min(1440, status_.remainingMinutes));
  }
}

void Battery::provideInitialEstimate() {
  // Provide rough estimate based on typical battery behavior
  // Assume average discharge rate of 1% per 12 minutes (5 hours for full battery)
  const double TYPICAL_DISCHARGE_RATE = 1.0 / 12.0; // percent per minute
  const double TYPICAL_CHARGE_RATE = 1.0 / 3.0;     // percent per minute (faster charging)
  
  if (status_.isCharging) {
    int remainingPercent = 100 - status_.percentage;
    status_.remainingMinutes = static_cast<int>(remainingPercent / TYPICAL_CHARGE_RATE);
  } else {
    status_.remainingMinutes = static_cast<int>(status_.percentage / TYPICAL_DISCHARGE_RATE);
  }
  
  status_.hasRuntimeEstimate = true;
  status_.remainingMinutes = std::max(0, std::min(1440, status_.remainingMinutes));
}

void Battery::provideSimpleEstimate() {
  if (history_.size() < 2) {
    provideInitialEstimate();
    return;
  }
  
  // Use available history but with less precision
  auto oldest = history_.front();
  auto newest = history_.back();
  
  auto timeDiff = std::chrono::duration_cast<std::chrono::minutes>(newest.timestamp - oldest.timestamp).count();
  if (timeDiff <= 0) {
    provideInitialEstimate();
    return;
  }
  
  int percentageDiff = newest.percentage - oldest.percentage;
  double rate = static_cast<double>(percentageDiff) / static_cast<double>(timeDiff);
  
  if (status_.isCharging && rate > 0.01) {
    int remainingPercent = 100 - status_.percentage;
    status_.remainingMinutes = static_cast<int>(remainingPercent / rate);
    status_.hasRuntimeEstimate = true;
  } else if (!status_.isCharging && rate < -0.01) {
    status_.remainingMinutes = static_cast<int>(status_.percentage / (-rate));
    status_.hasRuntimeEstimate = true;
  } else {
    // No clear trend, use typical estimate
    provideInitialEstimate();
    return;
  }
  
  status_.remainingMinutes = std::max(0, std::min(1440, status_.remainingMinutes));
}

}  // namespace core
}  // namespace viz1090