#pragma once
#include <vector>
#include <memory>
#include <string>
#include "GameObject.h"

class SceneSerializer {
public:
    static void SaveScene(const std::string& filepath, const std::vector<std::shared_ptr<GameObject>>& scene);
    static void LoadScene(const std::string& filepath, std::vector<std::shared_ptr<GameObject>>& scene);
};
