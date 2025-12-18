#include "AssetDatabase.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

AssetDatabase& AssetDatabase::instance() {
    static AssetDatabase inst;
    return inst;
}

void AssetDatabase::initialize(const std::string& assetsPath, const std::string& libraryPath) {
   assets_path_ = assetsPath;
    library_path_ = libraryPath;

    // Create directories if they don't exist
 fs::create_directories(fs::path(assetsPath));
    fs::create_directories(fs::path(libraryPath));

    LOG_INFO("AssetDatabase initialized. Assets: " + assetsPath + ", Library: " + libraryPath);
    
    // Initial refresh
    refresh();
}

void AssetDatabase::shutdown() {
    assets_.clear();
    guid_to_index_.clear();
    path_to_index_.clear();
    LOG_INFO("AssetDatabase shut down.");
}

void AssetDatabase::refresh() {
    assets_.clear();
 guid_to_index_.clear();
    path_to_index_.clear();

    LOG_INFO("AssetDatabase: Scanning Assets folder...");
 scanAssetsFolder();
    ensureMetaFiles();
    LOG_INFO("AssetDatabase: Refresh complete. Found " + std::to_string(assets_.size()) + " assets.");
}

std::string AssetDatabase::determineAssetType(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) return "Unknown";

    std::string ext = filename.substr(dotPos);
    
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") return "Model";
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".bmp") return "Texture";
    
    return "Unknown";
}

void AssetDatabase::scanAssetsFolder() {
 if (!fs::exists(assets_path_)) {
        LOG_WARN("Assets folder does not exist: " + assets_path_);
     return;
    }

    for (const auto& entry : fs::recursive_directory_iterator(assets_path_)) {
        if (entry.is_regular_file()) {
      std::string filename = entry.path().filename().string();
   
      // Skip .meta files
            if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".meta") continue;

            loadAssetMetadata(entry.path());
        }
    }
}

void AssetDatabase::loadAssetMetadata(const fs::path& assetPath) {
    std::string sourcePath = assetPath.string();
    std::string metaPath = sourcePath + ".meta";

    AssetMeta meta;
 meta.sourcePath = sourcePath;
    meta.assetType = determineAssetType(assetPath.filename().string());

    // Try to load existing metadata
    if (fs::exists(metaPath)) {
        AssetMeta::loadFromFile(metaPath, meta);
    } else {
        // Create new metadata
      meta.guid = AssetMeta::generateGUID();
        meta.libraryPath = library_path_ + "/" + meta.guid;
        meta.referenceCount = 0;
        if (meta.assetType == "Texture") {
            meta.texMinFilter = "Linear";
            meta.texMagFilter = "Linear";
            meta.texWrapS = "Repeat";
            meta.texWrapT = "Repeat";
            meta.texFlipX = false;
            meta.texFlipY = false;
            meta.texMipmaps = true;
        } else if (meta.assetType == "Model") {
            meta.meshScale = 1.0;
            meta.axisUp = "Y";
            meta.axisForward = "-Z";
            meta.ignoreCameras = true;
            meta.ignoreLights = true;
        }
    }

    // Update timestamp
    meta.sourceTimestamp = fs::last_write_time(assetPath).time_since_epoch().count();

    // Add to database
    size_t index = assets_.size();
    guid_to_index_[meta.guid] = index;
    path_to_index_[meta.sourcePath] = index;
    assets_.push_back(meta);
}

void AssetDatabase::ensureMetaFiles() {
    for (auto& meta : assets_) {
     std::string metaPath = meta.sourcePath + ".meta";
        
        // Save metadata file
        if (!fs::exists(metaPath) || true) {  // Always update
            AssetMeta::saveToFile(metaPath, meta);
        }

 // Copy to library if not present
        if (!fs::exists(meta.libraryPath)) {
   copyAssetToLibrary(meta);
        }
    }
}

bool AssetDatabase::copyAssetToLibrary(const AssetMeta& meta) {
    try {
 fs::create_directories(fs::path(meta.libraryPath).parent_path());
        fs::copy_file(meta.sourcePath, meta.libraryPath, fs::copy_options::overwrite_existing);
        LOG_INFO("Copied asset to library: " + meta.libraryPath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to copy asset to library: " + std::string(e.what()));
        return false;
    }
}

bool AssetDatabase::importAsset(const std::string& sourcePath) {
    if (!fs::exists(sourcePath)) {
        LOG_ERROR("Source asset does not exist: " + sourcePath);
        return false;
 }

 // Check if already imported
 if (findAssetBySourcePath(sourcePath)) {
  LOG_WARN("Asset already imported: " + sourcePath);
        return false;
    }

    // Load and register the asset
    loadAssetMetadata(fs::path(sourcePath));
    ensureMetaFiles();

    LOG_INFO("Asset imported: " + sourcePath);
    return true;
}

bool AssetDatabase::deleteAsset(const std::string& sourcePath) {
   AssetMeta* meta = findAssetBySourcePath(sourcePath);
    if (!meta) {
   LOG_ERROR("Asset not found: " + sourcePath);
        return false;
    }

    if (meta->referenceCount > 0) {
        LOG_ERROR("Cannot delete asset with active references: " + sourcePath);
 return false;
    }

    try {
        // Delete source file
      if (fs::exists(sourcePath)) {
            fs::remove(sourcePath);
        }

// Delete metadata file
       std::string metaPath = sourcePath + ".meta";
        if (fs::exists(metaPath)) {
    fs::remove(metaPath);
        }

        // Delete library file
  if (fs::exists(meta->libraryPath)) {
    fs::remove(meta->libraryPath);
        }

        // Remove from database
    auto it = guid_to_index_.find(meta->guid);
    if (it != guid_to_index_.end()) {
     size_t idx = it->second;
          assets_.erase(assets_.begin() + idx);
      guid_to_index_.erase(it);
            path_to_index_.erase(meta->sourcePath);
    }

        LOG_INFO("Asset deleted: " + sourcePath);
        return true;
    } catch (const std::exception& e) {
  LOG_ERROR("Failed to delete asset: " + std::string(e.what()));
    return false;
    }
}

AssetMeta* AssetDatabase::findAssetBySourcePath(const std::string& sourcePath) {
    auto it = path_to_index_.find(sourcePath);
    if (it != path_to_index_.end()) {
        return &assets_[it->second];
    }
    return nullptr;
}

AssetMeta* AssetDatabase::findAssetByGUID(const std::string& guid) {
    auto it = guid_to_index_.find(guid);
    if (it != guid_to_index_.end()) {
    return &assets_[it->second];
  }
    return nullptr;
}

void AssetDatabase::incrementReference(const std::string& guid) {
    AssetMeta* meta = findAssetByGUID(guid);
    if (meta) {
        meta->referenceCount++;
   LOG_INFO("Incremented reference count for asset " + guid + " to " + std::to_string(meta->referenceCount));
    }
}

void AssetDatabase::decrementReference(const std::string& guid) {
    AssetMeta* meta = findAssetByGUID(guid);
    if (meta && meta->referenceCount > 0) {
        meta->referenceCount--;
        LOG_INFO("Decremented reference count for asset " + guid + " to " + std::to_string(meta->referenceCount));
    }
}

int AssetDatabase::getReferenceCount(const std::string& guid) const {
    auto it = guid_to_index_.find(guid);
    if (it != guid_to_index_.end()) {
        return assets_[it->second].referenceCount;
    }
return 0;
}
