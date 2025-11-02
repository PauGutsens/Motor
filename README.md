# Motor - 3D Game Engine
**Version 0.1**

Motor is is a 3D game engine developed as an academic project for the Game Engine subject at the Videogame Design and Development degree at UPC, CITM (Terrassa, Spain). The engine is built with C++ using OPneGL and provides an editor with support for the FBX models, textures and gameobjects real-time manipulation.

**🔗 Project Github:** [https://github.com/PauGutsens/Motor](https://github.com/PauGutsens/Motor)

### Dependencies Used

- **SDL3**
- **OpenGL** 
- **GLEW**
- **ImGui**
- **Assimp**
- **DevIL**
- **GLM**


## 1. How to use the engine

### Camera controls:
- **Holding Right Click + Moving Mouse**: Camera rotates (free look).
- **Holding Right Click + W/A/S/D**: FPS-like movement.
  - **W**: Forward.
  - **S**: Backwards.
  - **A**: Left.
  - **D**: Right.
- **Holding Right Click + SHIFT + W/A/S/D**: Movement velovity x2.
- **Mouse Wheel**: Zoom in / zoom out.
- **Alt + Left Click + Drag**: Orbit around the selected object.
- **Clic Central del Ratón + Arrastrar**: Camera displace.
- **Clic Derecho + Shift + Arrastrar**: Alternative Camera displace.
- **Tecla F**: Focus on the selected object.

### Load File (Drag & Drop)
- 3D Models
	- Drag the file .fbx to the window and drop it, will load the model instanly.
	- It will create a GameObject for every mesh there is in the file.
- Textures
	- Select and object.
	- Drag the file .png/.jpg/.dds to the window and drop it Applies the texture to the selected object.
	- In case there's no selected object, will appear a message in the console.
	- Soported formats: PNG, JPG, JPEG, DDS.

- Object Selection
	- **Left Click**: Cycle between all GameObjects in the scene.
	- The selected GameObject will appear in yellow.
	- The object will appear selected in blue in the Inspctor window.

---

## 2. Ventanas del Editor

### Console: Gives logs of the system in real-time:
- Geometry Load (Assimp)
- Textures Load (DevIL)
- Library inicialization
- Warnings and errors
- Search filters
- Auto-scroll
- Clear button

### Configuration
- FPS graph in real time
- Hardware information:
  - GPU
  - Vendor
  - OpneGL version
- System information:
  - CPU core
  - System Ram

### Hierarchy
- GameObject name.
- Click for selection.

### Inspector
- Transform
- Position: Editable with DragFloat3 + reset button.
- Rotation: Editable with DragFloat3 incremental + reset button.
- Scale: Editable con DragFloat3 + reset button.
- All variables are editables in real time.
- Mesh:
	- Vertices: Shows the total number of vertices.
	- Triangles: Shows the total number of triangles.
	- Show Vertex Normals: Checkbox to visualize normals per vertice.
	- Show Face Normals: Checkbox to visualize normals per face.
	- Normal Length: Slider to adjust length of normals (0.01 - 2.0).
- Texture:
	- Texture ID: ID from OpenGL of the texture.
	- Size: Dimensions of the texture.
	- Apply Checkerboard: Applies test textures.
	- Restore Texture: Restore the original texture.
	- Preview: Preview of the applied texture.

---

## 3. Barra de Menú

### File
- Exit: Closes the engine.

### View
- Console: Shows/hides the console window.
- Config: Shows/hides the configuration window.
- Hierarchy: Shows/hides the hierarchy window.
- Inspector: Shows/hides the inspector window.

### Primitives
- Load basic geometry:
	- Cube, Sphere, Cone, Cylinder, Torus, Plane, Disc

### Help
- GitHub Docs: Opens the document in the browser.
- Report a bug: Opens a pages of issues in Github.
- Download latest: Opens the pages of releases in Github.
- About: Shows a window with engine information (Name, version, libraries, License MIT).

---

### Team Members:

- **Edu García** - [GitHub](https://github.com/Eduuuuuuuuuuuu)
- **Ao Yunqian** - [GitHub](https://github.com/YunqianAo)
- **Pau Hernández** - [GitHub](https://github.com/PauHeer)
- **Pau Gutsens** - [GitHub](https://github.com/PauGutsens)
