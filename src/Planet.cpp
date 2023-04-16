#include <Planet.hpp>

#define GL_ERROR_CHECK(A) \
do { \
    A; \
    GLenum error = glGetError(); \
    if(error != GL_NO_ERROR) { \
        std::cout << "Opengl error: " << error << std::endl; \
        std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl; \
    }\
} while(0)

void PlanetModel::generateVertexData()
    {
        float r = 7;
        int longitudeSegments = 100;
        int latitudeSegments = 100;

        float x,y,z;
        float u,v;
        float nx,ny,nz;

        unsigned int bottomLeft, topLeft, bottomRight, topRight;

        const float PI =  3.141592;

        for(int i=0;i<=latitudeSegments;++i) {
            for(int j=0;j<=longitudeSegments;++j) {
                const float theta = (1.0*j / longitudeSegments)*2*PI;
                const float phi = (1.0*i/latitudeSegments) * PI;

                x = r*sin(phi)*cos(theta);
                y = r*cos(phi);
                z = r*sin(phi)*sin(theta);

                nx = x / r;
                ny = y / r;
                nz = z / r;

                u = 1.0*j / longitudeSegments;
                v = 1.0*i / latitudeSegments;

                //bottomLeft = (i * (longitudeSegments+1)) + j;
                //topLeft = bottomLeft +longitudeSegments + 1;
                //bottomRight = bottomLeft + 1;
                //topRight = topLeft + 1;

                topLeft = (i * (longitudeSegments)) + j;
                bottomLeft = topLeft +longitudeSegments ;
                topRight = topLeft + 1;
                bottomRight = bottomLeft + 1;

                indices.push_back(bottomLeft);
                indices.push_back(topLeft);
                indices.push_back(bottomRight);

                indices.push_back(topLeft);
                indices.push_back(topRight);
                indices.push_back(bottomRight);

                //indices.push_back(topLeft);
                //indices.push_back(bottomLeft);
                //indices.push_back(bottomRight);

                //indices.push_back(bottomRight);
                //indices.push_back(topRight);
                //indices.push_back(topLeft);

                data.push_back(x);
                data.push_back(y);
                data.push_back(z);
                data.push_back(nx);
                data.push_back(ny);
                data.push_back(nz);
                data.push_back(u);
                data.push_back(v);


            }
        }

        //printf("Indices size %d, data size %d\n", indices.size(), data.size());
    }


void PlanetModel::setupBuffers()
{
    GL_ERROR_CHECK(glGenBuffers(1, &VBO));
    GL_ERROR_CHECK(glBindBuffer(GL_ARRAY_BUFFER, VBO));
    GL_ERROR_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * data.size(), &data[0], GL_STATIC_DRAW));

    GL_ERROR_CHECK(glGenBuffers(1, &EBO));
    GL_ERROR_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO));
    GL_ERROR_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW));

    GL_ERROR_CHECK(glGenVertexArrays(1, &VAO));
    GL_ERROR_CHECK(glBindVertexArray(VAO));

    GL_ERROR_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0));
    GL_ERROR_CHECK(glEnableVertexAttribArray(0));

    GL_ERROR_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float))));
    GL_ERROR_CHECK(glEnableVertexAttribArray(1));

    GL_ERROR_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float))));
    GL_ERROR_CHECK(glEnableVertexAttribArray(2));

    // Setup textures if provided
    if(texturePath != "") {
        int width, height, nChannels;
        unsigned char *data = stbi_load(texturePath.c_str(), &width, &height, &nChannels, 0);

        GLenum format;
        if (nChannels == 1)
            format = GL_RED;
        else if (nChannels == 3)
            format = GL_RGB;
        else if (nChannels == 4)
            format = GL_RGBA;

        GL_ERROR_CHECK(glGenTextures(1, &texture));
        GL_ERROR_CHECK(glBindTexture(GL_TEXTURE_2D, texture));

        GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

        if(data) {
            GL_ERROR_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data));
            stbi_image_free(data);
        }


    }

    // Unbind
    GL_ERROR_CHECK(glBindVertexArray(0));
    GL_ERROR_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_ERROR_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));


}

void PlanetModel::draw()
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    GL_ERROR_CHECK(glBindVertexArray(VAO));

    if(hasTexture()) {
        GL_ERROR_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
    }
    
    GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, &indices[0]));

    glDisable(GL_CULL_FACE);
}
