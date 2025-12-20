#pragma once
#include <memory>
#include <string>
#include <vector>

class GameObject;

namespace SceneIO
{
    // Text-based custom scene format (easy to debug + no extra deps).
    // Stores: hierarchy, name, 4x4 transform matrix, optional model reference + mesh index.
    //
    // IMPORTANT:
    // - assetsRoot: absolute path to your Assets folder (used only when loading model files).
    // - modelRef stored in file is relative to Assets (e.g. "Street/Street environment_V01.FBX").
    //
    // Return false on failure; optionally fill outError with human-readable reason.
    bool saveToFile(
        const std::string& filePath,
        const std::vector<std::shared_ptr<GameObject>>& roots,
        std::string* outError = nullptr);

    bool loadFromFile(
        const std::string& filePath,
        std::vector<std::shared_ptr<GameObject>>& roots,
        const std::string& assetsRoot,
        std::string* outError = nullptr);

    // In-memory snapshot (used by Play/Stop to restore edit scene)
    std::string serialize(
        const std::vector<std::shared_ptr<GameObject>>& roots);

    bool deserialize(
        const std::string& text,
        std::vector<std::shared_ptr<GameObject>>& roots,
        const std::string& assetsRoot,
        std::string* outError = nullptr);
}
