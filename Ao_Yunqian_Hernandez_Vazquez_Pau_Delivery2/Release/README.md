\# MotorA2 – Custom Game Engine Editor



---



\### 1. Descripción del Proyecto



MotorA2 es un proyecto de motor de videojuegos y editor desarrollado en C++ como parte de la asignatura de Motores / Game Engines. El objetivo principal es comprender la arquitectura interna de un motor 3D mediante la implementación directa de sus sistemas principales.



---



\### 2. Controles del Editor

2.1 Cámara (Scene View)



Los controles están implementados en Editor/Camera.cpp:



RMB (botón derecho) + mover ratón: rotación 



W/A/S/D (solo mientras RMB está pulsado): mover cámara (adelante/izquierda/atrás/derecha)



Q/E (solo mientras RMB está pulsado): bajar/subir



Shift + RMB + mover ratón: pan (desplazar lateralmente)



MMB (botón medio) + arrastrar: pan



Alt + LMB (botón izquierdo) + arrastrar: orbit (orbitar alrededor del objetivo)



Rueda del ratón: dolly (zoom in/out)



2.2 Selección



Clic izquierdo en el viewport: seleccionar GameObject (si no estás sobre el Gizmo y no estás usando Alt)



F: “Frame Selected” (centrar la cámara en el objeto seleccionado)



Nota: el “ViewCube” (arriba a la derecha del viewport) permite cambiar la orientación de la cámara.



2.3 Gizmos (ImGuizmo)



Con el viewport enfocado:



W: Translate (mover)



E: Rotate (rotar)



R: Scale (escalar)



Q: alternar World/Local



---



\### 3. Funcionalidades Implementadas



\- Motor 3D básico con OpenGL

\- Sistema de escenas y GameObjects

\- Cámara de editor

\- Interfaz gráfica con ImGui

\- Gizmos de transformación (ImGuizmo)

\- Carga de modelos y texturas

\- Ejecutable Release funcional



\### Lista de tareas 

A. Editor básico y escena



&nbsp;Cargar automáticamente la escena por defecto al iniciar (StreetEnvironment / Street environment V01.FBX)(Ao)



&nbsp;Jerarquía: crear/eliminar GameObjects (Ao)



&nbsp;Inspector: Transform editable (Posición/Rotación/Escala)(Ao)



&nbsp;Selección con ratón en la escena (picking) (Ao)


&nbsp;Posicionar correctamente los modelos al hacer Load Street (Pau)





B. Cámara y vista



&nbsp;Cámara del editor: controles Orbit / Pan / Zoom (Pau)



&nbsp;Tecla “F” para enfocar al objeto seleccionado (Frame/Focus)(Pau)



&nbsp;Cambio rápido de vista (cubo/axes de orientación) (Ao)



C. Gizmo (manipulación de transform)



&nbsp;Gizmo: mover (Translate) (Pau)



&nbsp;Gizmo: rotar (Rotate) (Pau)



&nbsp;Gizmo: escalar (Scale) (Pau)



&nbsp;Alternar coordenadas Local/Mundo (Pau)



D. Guardado y carga de escena



&nbsp;Serializar la escena a un formato propio (por ejemplo .m2scene) (Ao)



&nbsp;Deserializar y cargar la escena (Ao)



&nbsp;Acceso a guardar/cargar desde la UI del editor (Ao)



E. Sistema de recursos (Assets / Library)



&nbsp;Ventana de Assets (Ao)



&nbsp;Importar modelos/texturas a Assets (Ao)



&nbsp;Convertir recursos importados a formato propio y guardarlos en Library (por ejemplo .myMesh) (Pau)



F. Culling y estructura de aceleración



&nbsp;Frustum Culling (Ao)



G. Control de simulación



&nbsp;Play / Pause / Stop (Ao)



H. Entrega y publicación (Ao)



&nbsp;Ejecutable Release (MotorA2.exe) + DLLs necesarias



&nbsp;Estructura del paquete de entrega según el PDF (solo exe/dll, Assets, Library, README, Licencias)



&nbsp;README (controles, lista de funcionalidades, completado/no completado)



&nbsp;Archivo LICENSE



&nbsp;Carpeta Licencias (licencias de librerías externas)



&nbsp;ZIP < 200 MB



&nbsp;Subir el mismo ZIP a GitHub Releases



---



\### 4. Compilación



El proyecto se compila usando CMake y vcpkg con Visual Studio 2022.



---



\### 5. Ejecución



Ejecutar `MotorA2.exe` desde la carpeta `Release` junto con sus DLLs.





