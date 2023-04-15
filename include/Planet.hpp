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

#include <vector>

class PlanetModel
{
    std::vector<float> data;
    std::vector<unsigned int> indices;
    unsigned int VBO, VAO, EBO;

    void generateVertexData();
    void setupBuffers();
public:

    void draw();

    PlanetModel()
    {
        this->generateVertexData();
        this->setupBuffers();
    }


};

#endif
