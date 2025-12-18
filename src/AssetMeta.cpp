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
        else if (key == "texMinFilter") outMeta.texMinFilter = value;
        else if (key == "texMagFilter") outMeta.texMagFilter = value;
        else if (key == "texWrapS") outMeta.texWrapS = value;
        else if (key == "texWrapT") outMeta.texWrapT = value;
        else if (key == "texFlipX") outMeta.texFlipX = (value == "true" || value == "1");
        else if (key == "texFlipY") outMeta.texFlipY = (value == "true" || value == "1");
        else if (key == "texMipmaps") outMeta.texMipmaps = (value == "true" || value == "1");
        else if (key == "meshScale") outMeta.meshScale = std::stod(value);
        else if (key == "axisUp") outMeta.axisUp = value;
        else if (key == "axisForward") outMeta.axisForward = value;
        else if (key == "ignoreCameras") outMeta.ignoreCameras = (value == "true" || value == "1");
        else if (key == "ignoreLights") outMeta.ignoreLights = (value == "true" || value == "1");
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
    file << "texMinFilter: " << meta.texMinFilter << "\n";
    file << "texMagFilter: " << meta.texMagFilter << "\n";
    file << "texWrapS: " << meta.texWrapS << "\n";
    file << "texWrapT: " << meta.texWrapT << "\n";
    file << "texFlipX: " << (meta.texFlipX ? "true" : "false") << "\n";
    file << "texFlipY: " << (meta.texFlipY ? "true" : "false") << "\n";
    file << "texMipmaps: " << (meta.texMipmaps ? "true" : "false") << "\n";
    file << "meshScale: " << meta.meshScale << "\n";
    file << "axisUp: " << meta.axisUp << "\n";
    file << "axisForward: " << meta.axisForward << "\n";
    file << "ignoreCameras: " << (meta.ignoreCameras ? "true" : "false") << "\n";
    file << "ignoreLights: " << (meta.ignoreLights ? "true" : "false") << "\n";

    file.close();
    return true;
}
