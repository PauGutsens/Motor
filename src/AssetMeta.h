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
    std::string texMinFilter = "Linear";
    std::string texMagFilter = "Linear";
    std::string texWrapS = "Repeat";
    std::string texWrapT = "Repeat";
    bool texFlipX = false;
    bool texFlipY = false;
    bool texMipmaps = true;
    double meshScale = 1.0;
    std::string axisUp = "Y";
    std::string axisForward = "-Z";
    bool ignoreCameras = true;
    bool ignoreLights = true;

    // Serialization helpers
    static bool loadFromFile(const std::string& metaFilePath, AssetMeta& outMeta);
    static bool saveToFile(const std::string& metaFilePath, const AssetMeta& meta);
    static std::string generateGUID();
};
