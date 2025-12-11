#pragma once
#include <string>
#include <cstdint>
#include <filesystem>

/**
 * Metadata for an asset file
 * Stores information about how an asset should be imported/processed
 */
struct AssetMeta {
    std::string guid;    // Unique identifier
    std::string sourcePath;   // Path in /Assets/
    std::string libraryPath;    // Path in /Library/
    int64_t sourceTimestamp = 0; // Last modification time of source
    std::string assetType;      // "Model", "Texture", "Primitive", etc.
    int referenceCount = 0;     // How many GameObjects use this asset

    // Serialization helpers
    static bool loadFromFile(const std::string& metaFilePath, AssetMeta& outMeta);
    static bool saveToFile(const std::string& metaFilePath, const AssetMeta& meta);
    static std::string generateGUID();
};
