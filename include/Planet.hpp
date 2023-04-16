#ifndef PLANET_H
#define PLANET_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>

#include <vector>
#include <string>

class PlanetModel
{
    std::vector<float> data;
    std::vector<unsigned int> indices;
    std::string texturePath;
    unsigned int VBO, VAO, EBO, texture;

    void generateVertexData();
    void setupBuffers();
public:

    void draw();

    PlanetModel(const std::string texturePath = "")
    :   texturePath(texturePath)
    {
        this->generateVertexData();
        this->setupBuffers();
    }

    bool hasTexture() { return texturePath != ""; }


};

#endif
