#pragma once
#include "AssetMeta.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

/**
 * Central asset database system
 * Manages asset discovery, importing, caching, and reference counting
 */
class AssetDatabase {
public:
    static AssetDatabase& instance();

    // Initialization
    void initialize(const std::string& assetsPath, const std::string& libraryPath);
  void shutdown();

    // Asset discovery and refreshing
    void refresh();  // Scan /Assets and regenerate /Library
    
    // Asset importing
    bool importAsset(const std::string& sourcePath);
    bool deleteAsset(const std::string& sourcePath);

    // Asset querying
    AssetMeta* findAssetBySourcePath(const std::string& sourcePath);
    AssetMeta* findAssetByGUID(const std::string& guid);
    const std::vector<AssetMeta>& getAllAssets() const { return assets_; }

    // Reference counting
    void incrementReference(const std::string& guid);
    void decrementReference(const std::string& guid);
    int getReferenceCount(const std::string& guid) const;

    // Getters
    const std::string& getAssetsPath() const { return assets_path_; }
  const std::string& getLibraryPath() const { return library_path_; }

    // Determine asset type from file extension
    static std::string determineAssetType(const std::string& filename);

private:
    AssetDatabase() = default;

    std::vector<AssetMeta> assets_;
std::unordered_map<std::string, size_t> guid_to_index_;  // For quick lookup
    std::unordered_map<std::string, size_t> path_to_index_;  // For quick lookup

    std::string assets_path_;
    std::string library_path_;

    // Helper functions
    void scanAssetsFolder();
    void ensureMetaFiles();
    bool copyAssetToLibrary(const AssetMeta& meta);
    void loadAssetMetadata(const std::filesystem::path& assetPath);
};
