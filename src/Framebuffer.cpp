#include "Framebuffer.h"

Framebuffer::Framebuffer() {
    
}

Framebuffer::~Framebuffer() {
    if (fboID) glDeleteFramebuffers(1, &fboID);
    if (textureID) glDeleteTextures(1, &textureID);
    if (rboID) glDeleteRenderbuffers(1, &rboID);
}

void Framebuffer::Init(int w, int h) {
    width = w;
    height = h;

    if (fboID) {
        glDeleteFramebuffers(1, &fboID);
        glDeleteTextures(1, &textureID);
        glDeleteRenderbuffers(1, &rboID);
    }

    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);

    // Color Attachment (Texture)
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

    // Depth/Stencil Attachment (Renderbuffer)
    glGenRenderbuffers(1, &rboID);
    glBindRenderbuffer(GL_RENDERBUFFER, rboID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboID);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Rescale(int w, int h) {
    if (width == w && height == h) return;
    Init(w, h);
}

void Framebuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glViewport(0, 0, width, height);
}

void Framebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
