#pragma once
#include <string>
#include <cstdint>

// �ɹ����� OpenGL ����ID(>0)��ʧ�ܷ��� 0
unsigned int LoadTexture2D(const std::string& path);

// ���ڳ�������������һ�Σ�������Ѿ��ڱ� ilInit() �������Բ�����
void EnsureDevILInited();
