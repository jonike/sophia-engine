#ifndef MESH_H
#define MESH_H

/**
 * Define a mesh of vertices and their attributes, stored in VBO's and attached to a VAO
 * Meshes can be used for many purposes: tile maps, sprites
 */

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

template <typename T>
struct VBOComponents {
   enum {NumComponents = 0};
};

template <>
struct VBOComponents<float> {
   enum {NumComponents = 1};
};

template <>
struct VBOComponents<glm::vec2> {
   enum {NumComponents = 2};
};

template <>
struct VBOComponents<glm::vec3> {
   enum {NumComponents = 3};
};

template <>
struct VBOComponents<glm::vec4> {
   enum {NumComponents = 4};
};

class Mesh
{
public:
    Mesh () {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    }
    ~Mesh () {
        for (auto vbo : vbos) {
            glDeleteBuffers(1, &vbo);
        }
    }

    template <typename T>
    unsigned addBuffer (const std::vector<T>& data, bool vertices=false) {
        GLuint id = vbos.size();
        GLuint vbo;
        // Create and bind the new buffer
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // Copy the vertex data to the buffer
        auto vertexData = reinterpret_cast<const float*>(data.data());
        glBufferData(GL_ARRAY_BUFFER, data.size() * VBOComponents<T>::NumComponents * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
        // Specify that the buffer data is going into attribute index 0, and contains N floats per vertex
        glVertexAttribPointer(id, VBOComponents<T>::NumComponents, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(id);
        if (vertices) {
            count = data.size();
        }
        vbos.push_back(vbo);
        return id;
    }

    template <typename T>
    void setBuffer (unsigned id, const std::vector<T>& data) {
        GLuint vbo = vbos[id];
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // Copy the vertex data to the buffer
        auto vertexData = reinterpret_cast<const float*>(data.data());
        glBufferData(GL_ARRAY_BUFFER, data.size() * VBOComponents<T>::NumComponents * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
    }

    void set (unsigned id, bool enabled) {
        if (enabled) {
            glEnableVertexAttribArray(id);
        } else {
            glDisableVertexAttribArray(id);
        }
    }

    unsigned addIndexBuffer () {
        GLuint id = vbos.size();
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        vbos.push_back(ibo);
        return id;
    }

    inline void draw () {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, count);
    }
    inline void draw (unsigned int instances) {
        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, count, instances);
    }
    inline void drawIndexed (const std::vector<GLushort>& indices)
    {
        glBindVertexArray(vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        auto indexData = reinterpret_cast<const GLushort*>(indices.data());
        auto size = indices.size() * sizeof(GLushort);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indexData, GL_STREAM_DRAW);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);
    }

private:
    GLuint count;
    GLuint vao;
    GLuint ibo;
    std::vector<GLuint> vbos;
};

#endif // MESH_H
