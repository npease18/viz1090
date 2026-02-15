#ifndef REGISTRATION_LOOKUP_H
#define REGISTRATION_LOOKUP_H

#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace viz1090 {
namespace core {

/// Registration data entry
struct RegistrationData {
    std::string registration;  // Aircraft registration (e.g., "N1234A")
    std::string type;         // Aircraft type (e.g., "C172")
    std::string flag;         // Flag field from JSON
    std::string description;  // Description field from JSON
};

/// Aircraft registration lookup service
/// 
/// Provides fast lookup of aircraft registration and type information
/// from ICAO address using the registration_data JSON files.
class RegistrationLookup {
public:
    /// Constructor
    RegistrationLookup();
    
    /// Destructor
    ~RegistrationLookup();

    /// Initialize the lookup system with registration data directory
    /// @param dataPath Path to the registration_data directory
    /// @return true if initialization succeeded, false otherwise
    bool initialize(const std::string& dataPath);

    /// Lookup aircraft type by ICAO address
    /// @param icaoAddr ICAO address (24-bit hex value)
    /// @return Aircraft type string (e.g., "C172") or empty string if not found
    std::string lookupAircraftType(uint32_t icaoAddr) const;

    /// Lookup full registration data by ICAO address
    /// @param icaoAddr ICAO address (24-bit hex value)
    /// @param data Output registration data, only valid if function returns true
    /// @return true if aircraft was found, false otherwise
    bool lookupRegistrationData(uint32_t icaoAddr, RegistrationData& data) const;

    // Helper methods for testing
    std::string icaoToHex(uint32_t icaoAddr) const;
    std::string getFilePrefix(const std::string& icaoHex) const;

private:
    struct FileCache {
        std::unordered_map<std::string, RegistrationData> entries;
        bool loaded = false;
    };

    /// Load a specific JSON file into cache
    /// @param filePrefix The file prefix (e.g., "A0")
    /// @return true if loaded successfully
    bool loadRegistrationFile(const std::string& filePrefix);

    /// Parse a JSON registration file
    /// @param filePath Path to the JSON file
    /// @param filePrefix The hex prefix for this file
    /// @return true if parsed successfully
    bool parseRegistrationFile(const std::string& filePath, const std::string& filePrefix);

    std::string dataPath_;
    mutable std::unordered_map<std::string, std::unique_ptr<FileCache>> fileCache_;
    bool initialized_ = false;
};

} // namespace core
} // namespace viz1090

#endif // REGISTRATION_LOOKUP_H