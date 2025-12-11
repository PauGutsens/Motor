#include "AssetMeta.h"
#include <fstream>
#include <random>
#include <sstream>
#include <chrono>
#include <sys/stat.h>

std::string AssetMeta::generateGUID() {
 static std::random_device rd;
    static std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    uint64_t guid = dis(gen);
    std::stringstream ss;
    ss << std::hex << guid;
    return ss.str();
}

bool AssetMeta::loadFromFile(const std::string& metaFilePath, AssetMeta& outMeta) {
    std::ifstream file(metaFilePath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

      // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

 if (key == "guid") outMeta.guid = value;
        else if (key == "sourcePath") outMeta.sourcePath = value;
        else if (key == "libraryPath") outMeta.libraryPath = value;
 else if (key == "assetType") outMeta.assetType = value;
        else if (key == "referenceCount") outMeta.referenceCount = std::stoi(value);
        else if (key == "sourceTimestamp") outMeta.sourceTimestamp = std::stoll(value);
    }

    file.close();
    return !outMeta.guid.empty();
}

bool AssetMeta::saveToFile(const std::string& metaFilePath, const AssetMeta& meta) {
    std::ofstream file(metaFilePath);
    if (!file.is_open()) return false;

    file << "# Asset metadata file\n";
    file << "guid: " << meta.guid << "\n";
    file << "sourcePath: " << meta.sourcePath << "\n";
    file << "libraryPath: " << meta.libraryPath << "\n";
    file << "assetType: " << meta.assetType << "\n";
    file << "referenceCount: " << meta.referenceCount << "\n";
    file << "sourceTimestamp: " << meta.sourceTimestamp << "\n";

    file.close();
    return true;
}
