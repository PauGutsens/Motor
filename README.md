# Motor - 3D Game Engine
**Version 0.2**

Motor is a 3D game engine developed as an academic project for the Game Engine subject in the Video Game Design and Development degree at UPC, CITM (Terrassa, Spain). The engine is built with C++ using OpenGL and provides an editor with support for FBX models, textures, real-time GameObject manipulation, and scene management.

**Project Github:** [https://github.com/PauGutsens/Motor](https://github.com/PauGutsens/Motor)

### Team Members
* **Edu García** - [GitHub](https://github.com/eduuu04)
* **Pau Gutsens** - [GitHub](https://github.com/PauGutsens)

---

## 1. How to Use the Engine

### Camera Controls (Scene View)
The editor uses an "FPS-style" camera for scene navigation:
* **Hold Right Click + Move Mouse**: Rotate camera (Free look).
* **Hold Right Click + W/A/S/D**: Move camera.
    * **W / S**: Forward / Backward.
    * **A / D**: Left / Right.
    * **Q / E**: Up / Down.
* **Mouse Wheel**: Zoom In / Zoom Out (Fast forward/backward movement).
* **F Key**: Focus. Centers the camera on the currently selected object.

### File Management (Drag & Drop)
The engine supports loading resources by dragging files directly from your file explorer or the "Assets" panel into the scene window or inspector.
* **3D Models (.fbx)**: Dragging a file into the scene loads the model and instantiates a new GameObject with its mesh.
* **Textures (.png, .jpg, .dds)**:
    * Drag a texture onto the "Inspector" window (Texture section) or directly onto a selected object in the scene to apply it.

### Selection and Manipulation
* **Left Click (in Scene)**: Selects a GameObject using Raycasting. The selected object will be highlighted.
* **Inspector**: Allows numerical modification of the Transform (Position, Rotation, Scale).

### Toolbar
Located at the top, controls the simulation state:
* **Play**: Saves the current scene state and starts the simulation (Game Mode).
* **Pause**: Pauses the simulation while maintaining state.
* **Step**: Advances a single frame while paused.
* **Stop**: Stops the simulation and restores the scene to its original state before Play was pressed.

---

## 2. Additional Features

In addition to the basic requirements, the following advanced features have been implemented:

### Octree (Spatial Partitioning)
An **Octree** data structure has been implemented to optimize spatial queries.
* **Culling**: The engine discards rendering of objects outside the camera frustum by querying the Octree.
* **Optimized Raycasting**: Mouse selection uses the Octree to filter candidates, improving performance in scenes with many objects.
* **Debug**: You can visualize the Octree structure via the `Config > Show AABBs` menu.

### Framebuffers & Viewports
Scene rendering ("Scene") and game rendering ("Game") are decoupled using **Framebuffer Objects (FBOs)**. This allows:
* Independent and resizable editor windows within the ImGui interface.
* Separation of the editor camera logic from the game camera logic ("Main Camera").

### Scene Serialization
The engine includes a serialization system (Save/Load).
* Upon entering **Play** mode, the scene is automatically serialized to a temporary file.
* Upon exiting Play mode (**Stop**), the scene is deserialized, restoring all objects and properties to their original state.

### Asset Database & Assets Panel
An **Assets** window has been implemented that:
* Scans and displays files from the project directory.
* Supports folder navigation and Drag & Drop of resources into the scene.
* Manages basic metadata via `.meta` files.

### Advanced Interface (ImGui Docking)
The editor uses the **Docking** branch of ImGui, allowing the user to rearrange, dock, and undock windows (Inspector, Hierarchy, Scene, Game, Console) to their preference.

---

## 3. Comments

Below are some points of interest regarding the technical implementation:

* **Octree (`Octree.cpp` / `Octree.h`)**: The `Octree` class handles dynamic object insertion. If a node exceeds the capacity limit (`MAX_OBJECTS`), it subdivides recursively. It is integrated into both the rendering loop (for Frustum Culling) and the input system (for Raycasting).
* **Camera Management**: The `Camera` class handles both the editor camera and GameObject camera components. The editor camera uses a free-look system (fly-cam), while the game camera can be controlled by scripts or components.
* **Game Loop & Time Step**: In `main.cpp`, the main loop calculates `deltaTime` to ensure smooth movement independent of FPS. The Play/Pause logic manages the update of this time to stop or advance the simulation step-by-step.
* **File Structure**:
    * `src/`: C++ source code.
    * `Assets/`: Root folder for resources (models, textures).
    * `Library/`: Internal storage for imported resources (metadata).

### Dependencies Used
* **SDL3**: Windows and input management.
* **OpenGL & GLEW**: Graphics rendering.
* **ImGui (Docking branch)**: Editor user interface.
* **Assimp**: 3D model loading.
* **DevIL**: Image and texture loading.
* **GLM**: Mathematics (vectors and matrices).