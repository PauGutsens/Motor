#pragma once
#include <string>
#include <cstdint>

// �ɹ����� OpenGL ����ID(>0)��ʧ�ܷ��� 0 Devuelve el ID de textura OpenGL (>0) en caso de exito y 0 en caso de fallo.
unsigned int LoadTexture2D(const std::string& path);

// ���ڳ�������������һ�Σ�������Ѿ��ڱ� ilInit() �������Բ����� Llamada una vez al inicio del programa
void EnsureDevILInited();
