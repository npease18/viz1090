#pragma once

#include <string>
#include <chrono>
#include <vector>

namespace viz1090 {
namespace core {

/// Battery information reader for Linux systems
/// Reads battery status from /sys/class/power_supply/BAT0/
class Battery {
public:
  struct Status {
    int percentage{0};            // Battery percentage (0-100)
    bool isCharging{false};       // True if battery is charging
    bool isPresent{false};        // True if battery is detected
    bool isValid{false};          // True if battery information was successfully read
    int remainingMinutes{-1};     // Estimated runtime in minutes (-1 = unknown)
    bool hasRuntimeEstimate{false}; // True if runtime estimate is available
  };

  Battery();

  /// Read current battery status
  /// Returns updated status information
  Status readStatus();

  /// Get the last read battery status without updating
  const Status& getStatus() const { return status_; }

private:
  Status status_;
  std::string batteryPath_;

  // Battery history for runtime estimation
  struct BatteryReading {
    int percentage;
    std::chrono::steady_clock::time_point timestamp;
  };
  
  std::vector<BatteryReading> history_;
  static constexpr size_t MAX_HISTORY_SIZE = 10; // Keep last 10 readings
  static constexpr std::chrono::minutes MIN_HISTORY_TIME{5}; // Need 5 min of data
  
  /// Read a value from a file in the battery path
  std::string readBatteryFile(const std::string& filename) const;
  
  /// Calculate runtime estimate based on discharge rate
  void calculateRuntimeEstimate();
  
  /// Update battery history with new reading
  void updateHistory(int percentage);
  
  /// Get average discharge rate over time (percentage per minute)
  double getDischargeRate() const;
  
  /// Provide initial rough estimate with minimal data
  void provideInitialEstimate();
  
  /// Provide simple estimate with limited historical data
  void provideSimpleEstimate();
};

}  // namespace core
}  // namespace viz1090