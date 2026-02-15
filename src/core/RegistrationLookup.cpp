#include "RegistrationLookup.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace viz1090 {
namespace core {

// Simple JSON parsing helpers - based on StyleManager approach
namespace {

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// Extract string value from JSON key:value pair
std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";

    auto startQuote = json.find('"', pos + 1);
    if (startQuote == std::string::npos) return "";

    auto endQuote = json.find('"', startQuote + 1);
    if (endQuote == std::string::npos) return "";

    return json.substr(startQuote + 1, endQuote - startQuote - 1);
}

} // anonymous namespace

RegistrationLookup::RegistrationLookup() = default;

RegistrationLookup::~RegistrationLookup() = default;

bool RegistrationLookup::initialize(const std::string& dataPath) {
    dataPath_ = dataPath;
    
    // Check if the directory exists
    if (!std::filesystem::exists(dataPath) || !std::filesystem::is_directory(dataPath)) {
        std::cerr << "Registration data directory not found: " << dataPath << std::endl;
        return false;
    }
    
    std::cerr << "Registration lookup initialized with: " << dataPath << std::endl;
    initialized_ = true;
    return true;
}

std::string RegistrationLookup::lookupAircraftType(uint32_t icaoAddr) const {
    RegistrationData data;
    if (lookupRegistrationData(icaoAddr, data)) {
        return data.type;
    }
    
    // Debug output for first few failed lookups
    static int debugFailCount = 0;
    if (debugFailCount < 3) {
        std::string icaoHex = icaoToHex(icaoAddr);
        std::string filePrefix = getFilePrefix(icaoHex);
        std::cerr << "Failed lookup for " << icaoHex << " (file: " << filePrefix << ".json)" << std::endl;
        debugFailCount++;
    }
    
    return "";
}

bool RegistrationLookup::lookupRegistrationData(uint32_t icaoAddr, RegistrationData& data) const {
    if (!initialized_) {
        return false;
    }

    std::string icaoHex = icaoToHex(icaoAddr);
    std::string filePrefix = getFilePrefix(icaoHex);
    std::string icaoSuffix = icaoHex.substr(2); // Remove file prefix part

    // Debug output for all lookups (first few)
    static int debugCount = 0;
    if (debugCount < 10) {
        std::cerr << "LOOKUP " << debugCount << ": ICAO=" << std::hex << icaoAddr 
                  << " hex=" << icaoHex << " prefix=" << filePrefix 
                  << " suffix=" << icaoSuffix << std::endl;
        debugCount++;
    }

    // Load file if not already cached
    if (fileCache_.find(filePrefix) == fileCache_.end()) {
        if (!const_cast<RegistrationLookup*>(this)->loadRegistrationFile(filePrefix)) {
            return false;
        }
    }

    auto& cache = fileCache_[filePrefix];
    if (!cache || !cache->loaded) {
        return false;
    }

    auto it = cache->entries.find(icaoSuffix);
    if (it != cache->entries.end()) {
        data = it->second;
        return true;
    }

    return false;
}

bool RegistrationLookup::loadRegistrationFile(const std::string& filePrefix) {
    std::string filePath = dataPath_ + "/" + filePrefix + ".json";
    
    if (!std::filesystem::exists(filePath)) {
        // File doesn't exist - this is normal for some prefixes
        return false;
    }

    return parseRegistrationFile(filePath, filePrefix);
}

bool RegistrationLookup::parseRegistrationFile(const std::string& filePath, const std::string& filePrefix) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    // Read entire file
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Create cache entry
    auto cache = std::make_unique<FileCache>();

    // Parse JSON content - look for entries like "E001":{"r":"N1556W","t":"AT5T","f":"00","desc":"AT-502"}
    size_t pos = 0;
    while (pos < content.length()) {
        // Find next entry key (quoted string followed by colon)
        auto keyStart = content.find('"', pos);
        if (keyStart == std::string::npos) break;

        auto keyEnd = content.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;

        auto colon = content.find(':', keyEnd);
        if (colon == std::string::npos) break;

        // Extract key
        std::string key = content.substr(keyStart + 1, keyEnd - keyStart - 1);

        // Find the object start and end
        auto objStart = content.find('{', colon);
        if (objStart == std::string::npos) break;

        // Find matching closing brace
        int braceCount = 1;
        size_t objEnd = objStart + 1;
        while (objEnd < content.length() && braceCount > 0) {
            if (content[objEnd] == '{') braceCount++;
            else if (content[objEnd] == '}') braceCount--;
            objEnd++;
        }

        if (braceCount != 0) break; // Malformed JSON

        // Extract object content
        std::string objContent = content.substr(objStart, objEnd - objStart);

        // Parse the registration entry
        RegistrationData regData;
        regData.registration = extractJsonString(objContent, "r");
        regData.type = extractJsonString(objContent, "t");
        regData.flag = extractJsonString(objContent, "f");
        regData.description = extractJsonString(objContent, "desc");

        // Store in cache
        cache->entries[key] = regData;

        // Continue search
        pos = objEnd;
    }

    cache->loaded = true;
    fileCache_[filePrefix] = std::move(cache);
    return true;
}

std::string RegistrationLookup::icaoToHex(uint32_t icaoAddr) const {
    // Convert to 6-digit uppercase hex string (e.g., 0xA00182 -> "A00182")
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << (icaoAddr & 0xFFFFFF);
    return ss.str();
}

std::string RegistrationLookup::getFilePrefix(const std::string& icaoHex) const {
    if (icaoHex.length() < 2) {
        return "";
    }
    
    // Return first 2 characters as file prefix
    return icaoHex.substr(0, 2);
}

} // namespace core
} // namespace viz1090