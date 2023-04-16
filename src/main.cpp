#include "learnopengl/mesh.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>

#include <Skybox.hpp>
#include <Planet.hpp>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

float orbitModifier = 1.0f;
float sunScaleModifier = 0;
float orbitScaleModifier = 1;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = true;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 sunPosition = glm::vec3(0.0f);
    float sunScale = 1.0f;
    float sunMass = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;
glm::vec3 centerOfMass;

class PlanetOrbit
{
public:
    float a,b,e,startTheta,speed;
    PlanetOrbit(float a, float b)
    : a(a), b(b) {
        startTheta = 1.0*random()/RAND_MAX * 10000;
        speed = 1.0/2 + 1.0/2 * 1.0*random()/RAND_MAX;

        //this->a *= 2;
        //this->b *= 2;

        //this->e = sqrt(this->a*this->a - this->b*this->b)/this->a;
        this->e = sqrt(a*a-b*b)/a;
    }

};

class Planet
{
    PlanetOrbit orbit;
    PlanetModel model;
    string texturePath;
    glm::vec3 position;
    float scale;
    float planetMass;
    bool sunPlanet;

public:

    Planet(string const texturePath, float a, float b, float scale = 0.1, float mass = 0.01, bool sunPlanet = false)
        : texturePath(texturePath),
          model(texturePath),
          orbit(PlanetOrbit(a,b)),
          position(glm::vec3(0,0,0)),
          scale(scale),
          sunPlanet(sunPlanet),
          planetMass(mass) {}

    Planet(const Planet& o)
        : texturePath(texturePath),
          orbit(PlanetOrbit(o.orbit.a,o.orbit.b)),
          position(o.position),
          scale(o.scale),
          planetMass(o.planetMass) {}

    PlanetModel& getModel() { return model; }
    const glm::vec3& getPosition() const { return position; }
    float getMass() const { return planetMass; }

    void Draw(Shader &shader)
    {
        float time = glfwGetTime();
        glm::mat4 planetModelMat = glm::mat4(1.0f);
        shader.use();

        float theta = orbit.speed*time + orbit.startTheta;
        float r = sqrt(1/(pow((cos(theta)/(orbit.a*orbitScaleModifier)),2) + pow(sin(theta)/(orbit.b*orbitModifier),2) ));
        float x = r*cos(theta);
        float z = r*sin(theta);

        // Revolution
        planetModelMat = glm::translate(planetModelMat, glm::vec3(x,0,z));
        planetModelMat = glm::translate(planetModelMat, centerOfMass + glm::vec3(orbit.e*orbit.a*orbitModifier,0,0));

        planetModelMat = glm::rotate(planetModelMat, glm::degrees(0.01f*time), glm::vec3(0,1.0,0));
        planetModelMat = glm::rotate(planetModelMat, glm::degrees(6*sin(orbit.startTheta)), glm::vec3(0,1.0,0));

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();


        shader.setMat4("model", planetModelMat);
        shader.setFloat("scale", scale + sunScaleModifier*sunPlanet);
        shader.setInt("HasTexture", (int)model.hasTexture());
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(view*planetModelMat)));
        shader.setMat3("normalMatrix", normalMatrix);

        position = glm::vec3(
            planetModelMat[3][0],
            planetModelMat[3][1],
            planetModelMat[3][2]
        );

        model.draw();
    }

    float getScale() const { return scale; }
};

void calculateCenterOfMass(const vector<Planet*> &planets)
{
    glm::vec3 res = programState->sunPosition * programState->sunMass;

    for(int i=0;i<planets.size();++i) {
        res += planets[i]->getPosition() * planets[i]->getMass();
    }

    centerOfMass = res;

}


void DrawImGui(ProgramState *programState);

int main() {
    srand(time(NULL));
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    programState->ImGuiEnabled = true;

    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader sunShader("resources/shaders/2.model_lighting.vs","resources/shaders/sun_shader.fs");
    Shader backpackShader("resources/shaders/backpack_shader.vs","resources/shaders/backpack_shader.fs");

    // load models
    // -----------
    Planet sunModel("resources/textures/sun.jpg", 1, 1, 0.3, 1, true);
    Planet earth("resources/textures/earth.jpg", 52, 50, 0.1);
    Planet mars("resources/textures/mars.jpg", 57, 55, 0.2,0.5);
    Planet venus("resources/textures/venus.jpg", 62, 60, 0.1);
    Planet jupiter("resources/textures/jupiter.jpg", 72, 70, 0.15);
    //sunModel.SetShaderTextureNamePrefix("material.");

    // Load backpack
    Model backpackModel("resources/objects/backpack/backpack.obj");
    backpackModel.SetShaderTextureNamePrefix("material.");


    std::vector<Planet*> planets {
        &earth, &mars, &venus, &jupiter
    };

    PointLight& pointLight = programState->pointLight;
    pointLight.position = programState->sunPosition;
    pointLight.ambient = glm::vec3(0.2);
    pointLight.diffuse = glm::vec3(1);
    pointLight.specular = glm::vec3(0.2);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.0009f;
    pointLight.quadratic = 0.00032f;



    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    calculateCenterOfMass(planets);

    vector<string> faces {
        "resources/skybox/blue/bkg1_right.png",
        "resources/skybox/blue/bkg1_left.png",
        "resources/skybox/blue/bkg1_top.png",
        "resources/skybox/blue/bkg1_bot.png",
        "resources/skybox/blue/bkg1_front.png",
        "resources/skybox/blue/bkg1_back.png",
    };
    Skybox skybox;
    skybox.Load(faces);

    Shader skyboxShader("resources/shaders/skybox.vs","resources/shaders/skybox.fs");

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw skybox
        skybox.Draw(programState->camera, skyboxShader);
        
        programState->sunPosition = sunModel.getPosition();
        pointLight.position = programState->sunPosition;

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations


        /*
        printf("Sun x y z: %.2f %.2f %.2f\n", 
            programState->sunPosition.x,
            programState->sunPosition.y,
            programState->sunPosition.z
        );

        printf("Light position x y z: %.2f %.2f %.2f\n",
            pointLight.position.x,
            pointLight.position.y,
            pointLight.position.z
        );
        */

        sunModel.Draw(sunShader);

        for(Planet *p : planets) {
            p->Draw(ourShader);
        }

        // Draw backpack
        {
            backpackShader.use();
            backpackShader.setVec3("pointLight.position", pointLight.position);
            backpackShader.setVec3("pointLight.ambient", pointLight.ambient);
            backpackShader.setVec3("pointLight.diffuse", pointLight.diffuse);
            backpackShader.setVec3("pointLight.specular", pointLight.specular);
            backpackShader.setFloat("pointLight.constant", pointLight.constant);
            backpackShader.setFloat("pointLight.linear", pointLight.linear);
            backpackShader.setFloat("pointLight.quadratic", pointLight.quadratic);
            backpackShader.setVec3("viewPosition", programState->camera.Position);
            backpackShader.setFloat("material.shininess", 32.0f);

            glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
            glm::mat4 view = programState->camera.GetViewMatrix();

            glm::mat4 model = glm::mat4(1);
            model = glm::translate(model, glm::vec3(0,5,0));

            backpackShader.setMat4("projection", projection);
            backpackShader.setMat4("view", view);
            backpackShader.setMat4("model", model);

            backpackModel.Draw(backpackShader);

        }

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {

    static const float _cooldown = 1.0f;
    static float _lastPressed = 0;
    static float _deltaTime = 0;

    _deltaTime = glfwGetTime() - _lastPressed;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && _deltaTime > _cooldown)  {
        if(programState->CameraMouseMovementUpdateEnabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            programState->CameraMouseMovementUpdateEnabled = false;
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            programState->CameraMouseMovementUpdateEnabled = true;
        }
        _lastPressed = glfwGetTime();
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Properties");
        ImGui::SliderFloat("Sunscale modifier", &sunScaleModifier, 0, 1.0);
        ImGui::SliderFloat("Orbit modifier", &orbitScaleModifier, 1, 3.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info - PRESS C TO FREEZE CAMERA");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Text("Camera locked: %d", programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}
