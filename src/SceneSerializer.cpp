#include "SceneSerializer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "ModelLoader.h"
#include "TextureLoader.h"
#include <filesystem>

// GLM includes for matrix decomposition
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

void SceneSerializer::SaveScene(const std::string& filepath, const std::vector<std::shared_ptr<GameObject>>& scene) {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for saving: " << filepath << std::endl;
        return;
    }

    for (size_t i = 0; i < scene.size(); ++i) {
        auto go = scene[i];
        out << "GameObject\n";
        out << "ID: " << i << "\n";
        out << "Name: " << go->name << "\n";
        
        // Decompose Transform Matrix
        glm::dvec3 scale;
        glm::dquat rotation;
        glm::dvec3 translation;
        glm::dvec3 skew;
        glm::dvec4 perspective;
        
        // Use the matrix from Transform
        glm::decompose(go->transform.mat(), scale, rotation, translation, skew, perspective);
        
        // Convert rotation to Euler angles (in degrees)
        glm::dvec3 euler = glm::degrees(glm::eulerAngles(rotation));

        out << "Position: " << translation.x << " " << translation.y << " " << translation.z << "\n";
        out << "Rotation: " << euler.x << " " << euler.y << " " << euler.z << "\n";
        out << "Scale: " << scale.x << " " << scale.y << " " << scale.z << "\n";
        
        // [NEW] Serialize Mesh Info
        if (!go->modelPath.empty()) {
            out << "ModelPath: " << go->modelPath << "\n";
            out << "ModelPath: " << go->modelPath << "\n";
            out << "MeshIndex: " << go->meshIndex << "\n";
        }
        
        // [NEW] Serialize Camera
        if (go->camera.enabled) {
            out << "Camera: true\n";
            out << "FOV: " << go->camera.fov << "\n";
            out << "Near: " << go->camera.zNear << "\n";
            out << "Far: " << go->camera.zFar << "\n";
            // Aspect is usually derived from window, but we can save it or default it
        }

        // Parent
        int parentIdx = -1;
        if (go->parent) {
             for(size_t j=0; j<scene.size(); ++j) {
                 if (scene[j].get() == go->parent) {
                     parentIdx = (int)j;
                     break;
                 }
             }
        }
        out << "ParentID: " << parentIdx << "\n";
        
        out << "--END--\n";
    }
    out.close();
}

void SceneSerializer::LoadScene(const std::string& filepath, std::vector<std::shared_ptr<GameObject>>& scene) {
    std::ifstream in(filepath);
    if (!in.is_open()) return;

    scene.clear(); 
    
    std::string line;
    std::shared_ptr<GameObject> currentGO = nullptr;
    
    struct ObjInfo {
        std::shared_ptr<GameObject> go;
        int parentID;
    };
    std::vector<ObjInfo> objects;

    vec3 loadedPos(0);
    vec3 loadedRot(0);
    vec3 loadedScale(1);

    while (std::getline(in, line)) {
        if (line == "GameObject") {
            currentGO = std::make_shared<GameObject>("Temp");
            objects.push_back({currentGO, -1});
            loadedPos = vec3(0); loadedRot = vec3(0); loadedScale = vec3(1);
        }
        else if (line.find("Name: ") == 0) {
            if(currentGO) currentGO->name = line.substr(6);
        }
        else if (line.find("Position: ") == 0) {
            std::stringstream ss(line.substr(10));
            double x, y, z; ss >> x >> y >> z;
            loadedPos = vec3(x,y,z);
            if(currentGO) currentGO->transform.setPosition(loadedPos);
        }
        else if (line.find("Rotation: ") == 0) {
            std::stringstream ss(line.substr(10));
            double x, y, z; ss >> x >> y >> z;
            loadedRot = vec3(x,y,z);
            if(currentGO) {
                currentGO->transform.resetRotation();
                currentGO->transform.rotateEulerDeltaDeg(loadedRot);
            }
        }
        else if (line.find("Scale: ") == 0) {
            std::stringstream ss(line.substr(7));
            double x, y, z; ss >> x >> y >> z;
            loadedScale = vec3(x,y,z);
            if(currentGO) currentGO->transform.setScale(loadedScale);
        }

        else if (line.find("ModelPath: ") == 0) {
            if(currentGO) currentGO->modelPath = line.substr(11);
        }
        else if (line.find("MeshIndex: ") == 0) {
            if(currentGO) currentGO->meshIndex = std::stoi(line.substr(11));
        }
        else if (line.find("Camera: ") == 0) {
            if(currentGO) currentGO->camera.enabled = true;
        }
        else if (line.find("FOV: ") == 0) {
            if(currentGO) currentGO->camera.fov = std::stod(line.substr(5));
        }
        else if (line.find("Near: ") == 0) {
            if(currentGO) currentGO->camera.zNear = std::stod(line.substr(6));
        }
        else if (line.find("Far: ") == 0) {
            if(currentGO) currentGO->camera.zFar = std::stod(line.substr(5));
        }
        else if (line.find("ParentID: ") == 0) {
            int pid = std::stoi(line.substr(10));
            if (!objects.empty()) objects.back().parentID = pid;
        }
    }
    
    // Reconstruct Hierarchy
    // Reconstruct Hierarchy and Restore Meshes
    // Simple Cache for models loaded during this restoration to avoid checking disk 100 times
    // Helper to find Assets folder (simpler version of main.cpp's logic)
    auto findAssetsPath = []() -> std::string {
        std::filesystem::path p = std::filesystem::current_path();
        for (int i = 0; i < 6; ++i) {
            if (std::filesystem::exists(p / "Assets") && std::filesystem::is_directory(p / "Assets")) {
                return (p / "Assets").string();
            }
            if (!p.has_parent_path()) break;
            p = p.parent_path();
        }
        return "Assets"; // Fallback
    };
    std::string assetsDir = findAssetsPath();
    std::unordered_map<std::string, std::vector<std::shared_ptr<Mesh>>> modelCache;

    for (size_t i = 0; i < objects.size(); ++i) {
        auto go = objects[i].go;
        scene.push_back(go);
        
        // Restore Mesh
        if (!go->modelPath.empty() && go->meshIndex >= 0) {
            // Check cache
            if (modelCache.find(go->modelPath) == modelCache.end()) {
                // Construct full path
                std::filesystem::path fullPath = std::filesystem::path(assetsDir) / go->modelPath;
                
                std::cout << "[SceneSerializer] Restoring model: " << fullPath.string() << std::endl;
                
                if (!std::filesystem::exists(fullPath)) {
                     std::cerr << "[SceneSerializer] ERROR: Model file not found: " << fullPath.string() << std::endl;
                     // Try absolute just in case it was stored absolute? No, main stores filename.
                } else {
                    auto meshes = ModelLoader::loadModel(fullPath.string()); 
                    modelCache[go->modelPath] = meshes;
                }
            }
            
            // Assign mesh if loaded
            if (modelCache.find(go->modelPath) != modelCache.end()) {
                const auto& meshes = modelCache[go->modelPath];
                if (go->meshIndex < (int)meshes.size()) {
                    go->setMesh(meshes[go->meshIndex]);
                } else {
                    std::cerr << "[SceneSerializer] ERROR: Mesh index " << go->meshIndex << " out of bounds for " << go->modelPath << std::endl;
                }
            }
        }

        int pid = objects[i].parentID;
        if (pid >= 0 && pid < (int)objects.size()) {
            objects[pid].go->addChild(go.get());
        }
    }
    
    in.close();
}
