#include "SceneIO.h"

#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <filesystem>

#include "GameObject.h"
#include "Renderer/ModelLoader.h"

namespace fs = std::filesystem;

namespace
{
    struct NodeRec
    {
        int id = -1;
        int parent = -1;
        std::string name;

        // row-major in file for readability (we'll write 16 numbers)
        mat4 mat = mat4(1.0);

        std::string modelRef; // relative to Assets
        int meshIndex = -1;
    };

    // ------- traversal -------
    void dfsCollect(GameObject* node, int parentId,
        std::vector<GameObject*>& outNodes,
        std::vector<int>& outParents)
    {
        if (!node) return;
        outParents.push_back(parentId);
        outNodes.push_back(node);
        const int myId = (int)outNodes.size() - 1;

        for (GameObject* c : node->children)
            dfsCollect(c, myId, outNodes, outParents);
    }

    static inline void setError(std::string* out, const std::string& msg)
    {
        if (out) *out = msg;
    }

    static inline std::string trim(const std::string& s)
    {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace((unsigned char)s[a])) ++a;
        while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
        return s.substr(a, b - a);
    }

    static bool parseInt(const std::string& s, int& out)
    {
        try {
            size_t pos = 0;
            int v = std::stoi(s, &pos);
            if (pos != s.size()) return false;
            out = v;
            return true;
        }
        catch (...) { return false; }
    }

    static bool parseMat16(const std::string& s, mat4& out)
    {
        std::istringstream iss(s);
        double v[16];
        for (int i = 0; i < 16; ++i)
        {
            if (!(iss >> v[i])) return false;
        }
        // file uses row-major (r0..r3), glm mat4 is column-major
        // Convert: out[col][row] = v[row*4 + col]
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                out[c][r] = v[r * 4 + c];
        return true;
    }

    static std::string matToTextRowMajor(const mat4& m)
    {
        std::ostringstream oss;
        oss.setf(std::ios::fixed);
        oss << std::setprecision(12);

        // write row-major for humans
        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                double v = m[c][r]; // column-major storage
                oss << v;
                if (!(r == 3 && c == 3)) oss << ' ';
            }
        }
        return oss.str();
    }

    static std::string escape(const std::string& s)
    {
        std::string o;
        o.reserve(s.size() + 8);
        for (char ch : s)
        {
            if (ch == '\\' || ch == '"') o.push_back('\\');
            o.push_back(ch);
        }
        return o;
    }

    static std::string unescape(const std::string& s)
    {
        std::string o;
        o.reserve(s.size());
        bool esc = false;
        for (char ch : s)
        {
            if (esc) { o.push_back(ch); esc = false; continue; }
            if (ch == '\\') { esc = true; continue; }
            o.push_back(ch);
        }
        return o;
    }

    // cache model loads: modelRef -> meshes
    using MeshList = std::vector<std::shared_ptr<Mesh>>;
    static std::unordered_map<std::string, MeshList> g_modelCache;
    static MeshList& getModelMeshes(const std::string& assetsRoot, const std::string& modelRef)
    {
        auto it = g_modelCache.find(modelRef);
        if (it != g_modelCache.end()) return it->second;

        fs::path full = fs::path(assetsRoot) / fs::path(modelRef);
        MeshList meshes = ModelLoader::loadModel(full.string());
        auto [insIt, ok] = g_modelCache.emplace(modelRef, std::move(meshes));
        return insIt->second;
    }

    static bool parse(const std::string& text, std::vector<NodeRec>& out, std::string* outError)
    {
        out.clear();
        std::istringstream iss(text);
        std::string line;

        // header
        if (!std::getline(iss, line)) { setError(outError, "Empty file"); return false; }
        line = trim(line);
        if (line.rfind("MotorA2Scene", 0) != 0)
        {
            setError(outError, "Invalid header (expected: MotorA2Scene v1)");
            return false;
        }

        NodeRec cur;
        bool inNode = false;

        auto flushNode = [&]() {
            if (!inNode) return;
            if (cur.id < 0) { /* ignore */ }
            else out.push_back(cur);
            cur = NodeRec{};
            inNode = false;
            };

        while (std::getline(iss, line))
        {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            if (line == "[Node]")
            {
                flushNode();
                inNode = true;
                cur = NodeRec{};
                continue;
            }
            if (line == "[/Node]")
            {
                flushNode();
                continue;
            }
            if (!inNode) continue;

            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = trim(line.substr(0, eq));
            std::string val = trim(line.substr(eq + 1));

            if (key == "id")
            {
                if (!parseInt(val, cur.id)) { setError(outError, "Bad id"); return false; }
            }
            else if (key == "parent")
            {
                if (!parseInt(val, cur.parent)) { setError(outError, "Bad parent"); return false; }
            }
            else if (key == "name")
            {
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
                    cur.name = unescape(val.substr(1, val.size() - 2));
                else
                    cur.name = val;
            }
            else if (key == "mat")
            {
                if (!parseMat16(val, cur.mat)) { setError(outError, "Bad mat (need 16 numbers)"); return false; }
            }
            else if (key == "model")
            {
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
                    cur.modelRef = unescape(val.substr(1, val.size() - 2));
                else
                    cur.modelRef = val;
            }
            else if (key == "meshIndex")
            {
                if (!parseInt(val, cur.meshIndex)) { setError(outError, "Bad meshIndex"); return false; }
            }
        }

        flushNode();
        return true;
    }

    static std::string write(const std::vector<NodeRec>& nodes)
    {
        std::ostringstream oss;
        oss << "MotorA2Scene v1\n";
        for (const auto& n : nodes)
        {
            oss << "[Node]\n";
            oss << "id=" << n.id << "\n";
            oss << "parent=" << n.parent << "\n";
            oss << "name=\"" << escape(n.name) << "\"\n";
            oss << "mat=" << matToTextRowMajor(n.mat) << "\n";
            if (!n.modelRef.empty())
                oss << "model=\"" << escape(n.modelRef) << "\"\n";
            if (n.meshIndex >= 0)
                oss << "meshIndex=" << n.meshIndex << "\n";
            oss << "[/Node]\n";
        }
        return oss.str();
    }
}

namespace SceneIO
{
    std::string serialize(const std::vector<std::shared_ptr<GameObject>>& roots)
    {
        std::vector<GameObject*> nodes;
        std::vector<int> parents;
        nodes.reserve(1024);
        parents.reserve(1024);

        for (const auto& r : roots)
            dfsCollect(r.get(), -1, nodes, parents);

        std::vector<NodeRec> recs;
        recs.reserve(nodes.size());

        for (int i = 0; i < (int)nodes.size(); ++i)
        {
            GameObject* go = nodes[i];
            NodeRec rec;
            rec.id = i;
            rec.parent = parents[i];
            rec.name = go->name;
            rec.mat = go->transform.mat();

            rec.modelRef = go->sourceModel;
            rec.meshIndex = go->sourceMeshIndex;

            recs.push_back(std::move(rec));
        }

        return write(recs);
    }

    bool deserialize(const std::string& text,
        std::vector<std::shared_ptr<GameObject>>& roots,
        const std::string& assetsRoot,
        std::string* outError)
    {
        std::vector<NodeRec> recs;
        if (!parse(text, recs, outError)) return false;

        int maxId = -1;
        for (auto& r : recs) maxId = std::max(maxId, r.id);
        if (maxId < 0) { roots.clear(); return true; }

        std::vector<std::shared_ptr<GameObject>> all((size_t)maxId + 1, nullptr);

        for (const auto& r : recs)
        {
            if (r.id < 0 || r.id >= (int)all.size()) continue;
            auto go = std::make_shared<GameObject>(r.name);
            go->transform.mat_mutable() = r.mat;
            go->parent = nullptr;
            go->children.clear();

            go->sourceModel = r.modelRef;
            go->sourceMeshIndex = r.meshIndex;

            all[(size_t)r.id] = go;
        }

        roots.clear();
        for (const auto& r : recs)
        {
            if (r.id < 0 || r.id >= (int)all.size()) continue;
            auto& go = all[(size_t)r.id];
            if (!go) continue;

            if (r.parent < 0)
            {
                roots.push_back(go);
            }
            else if (r.parent >= 0 && r.parent < (int)all.size() && all[(size_t)r.parent])
            {
                all[(size_t)r.parent]->addChild(go.get());
            }
            else
            {
                roots.push_back(go);
            }
        }

        for (auto& go : all)
        {
            if (!go) continue;
            if (go->sourceModel.empty() || go->sourceMeshIndex < 0) continue;

            auto& meshes = getModelMeshes(assetsRoot, go->sourceModel);
            if (go->sourceMeshIndex >= 0 && go->sourceMeshIndex < (int)meshes.size())
            {
                go->setMesh(meshes[(size_t)go->sourceMeshIndex]);
            }
        }

        return true;
    }

    bool saveToFile(const std::string& filePath,
        const std::vector<std::shared_ptr<GameObject>>& roots,
        std::string* outError)
    {
        std::ofstream ofs(filePath, std::ios::binary);
        if (!ofs)
        {
            setError(outError, std::string("Cannot open for write: ") + filePath);
            return false;
        }
        std::string text = serialize(roots);
        ofs.write(text.data(), (std::streamsize)text.size());
        return true;
    }

    bool loadFromFile(const std::string& filePath,
        std::vector<std::shared_ptr<GameObject>>& roots,
        const std::string& assetsRoot,
        std::string* outError)
    {
        std::ifstream ifs(filePath, std::ios::binary);
        if (!ifs)
        {
            setError(outError, std::string("Cannot open for read: ") + filePath);
            return false;
        }
        std::stringstream ss;
        ss << ifs.rdbuf();
        return deserialize(ss.str(), roots, assetsRoot, outError);
    }
}
