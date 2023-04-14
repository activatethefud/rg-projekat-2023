#include <Skybox.hpp>

int Skybox::Load(std::vector<std::string> &textureFaces)
{
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

    int _width, _height, _nrChannels;
    unsigned char *_data;

    stbi_set_flip_vertically_on_load(false);

    for(unsigned int i=0;i<textureFaces.size();++i) {
        _data = stbi_load(textureFaces[i].c_str(), &_width, &_height, &_nrChannels, 0);
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,GL_RGB,_width,_height,0,GL_RGB,GL_UNSIGNED_BYTE, _data
        );

    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    stbi_set_flip_vertically_on_load(true);


    return textureId;

}


void Skybox::Draw(Camera &camera, Shader &shader)
{
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);


    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            (float) 1200 / (float) 800, 0.1f, 100.0f);

    glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", skyboxView);
    glBindVertexArray(VAO);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}
