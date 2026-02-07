#pragma once

#include <string>

namespace viz1090 {
namespace core {

/// Battery information reader for Linux systems
/// Reads battery status from /sys/class/power_supply/BAT0/
class Battery {
public:
  struct Status {
    int percentage{0};        // Battery percentage (0-100)
    bool isCharging{false};   // True if battery is charging
    bool isPresent{false};    // True if battery is detected
    bool isValid{false};      // True if battery information was successfully read
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

  /// Read a value from a file in the battery path
  std::string readBatteryFile(const std::string& filename) const;
};

}  // namespace core
}  // namespace viz1090