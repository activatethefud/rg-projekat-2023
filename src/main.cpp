#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

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
    bool ImGuiEnabled = false;
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
    float a,b,e,startTheta;
    PlanetOrbit(float a, float b)
    : a(a), b(b) {
        this->e = sqrt(a*a - b*b)/a;
        startTheta = 1.0*random()/RAND_MAX * 10000;
    }

};

class Planet
{
    PlanetOrbit orbit;
    Model model;
    string modelPath;
    glm::vec3 position;
    float scale;
    float planetMass;

public:

    Planet(string const modelPath, float a, float b, float scale = 0.1, float mass = 0.01)
        : modelPath(modelPath),
          orbit(PlanetOrbit(a,b)),
          position(glm::vec3(0,0,0)),
          scale(scale),
          planetMass(mass) {
            this->model = Model(this->modelPath);
          }

    Planet(const Planet& o)
        : modelPath(o.modelPath),
          orbit(PlanetOrbit(o.orbit.a,o.orbit.b)),
          position(o.position),
          scale(o.scale),
          planetMass(o.planetMass) {
            this->model = Model(o.modelPath);
          }

    Model& getModel() { return model; }
    const glm::vec3& getPosition() const { return position; }
    float getMass() const { return planetMass; }

    void Draw(Shader &shader)
    {
        float time = glfwGetTime();
        glm::mat4 planetModelMat = glm::mat4(1.0f);

        float theta = time + orbit.startTheta;
        float r = sqrt(1/(pow((cos(theta)/orbit.a),2) + pow(sin(theta)/orbit.b,2) ));
        float x = r*cos(theta);
        float z = r*sin(theta);

        shader.setFloat("scale", scale);
        //planetModelMat = glm::scale(planetModelMat, glm::vec3(scale));
        planetModelMat = glm::translate(planetModelMat, glm::vec3(x,0,z));
        planetModelMat = glm::translate(planetModelMat, centerOfMass + glm::vec3(orbit.e*orbit.a,0,0));

        planetModelMat = glm::rotate(planetModelMat, glm::degrees(6*sin(orbit.startTheta)), glm::vec3(0,1.0,0));

        shader.setMat4("model", planetModelMat);

        position = glm::vec3(
            planetModelMat[3][0],
            planetModelMat[3][1],
            planetModelMat[3][2]
        );

        model.Draw(shader);
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
    //programState->LoadFromFile("resources/program_state.txt");
    programState->ImGuiEnabled = false;

    programState->camera.Position += glm::vec3(0,5,7);

    bool firstLook = true;

    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
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

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader sunShader("resources/shaders/sun_shader.vs","resources/shaders/sun_shader.fs");

    // load models
    // -----------
    Planet sunModel("resources/objects/sun/Sun.obj", 1, 1, 0.5, 1);
    Planet earth("resources/objects/sun/Sun.obj", 50, 45, 0.2);
    Planet mars("resources/objects/sun/Sun.obj", 50, 47, 0.2,0.5);
    Planet venus("resources/objects/sun/Sun.obj", 40, 40, 0.2);
    Planet neptune("resources/objects/sun/Sun.obj", 70, 40, 0.25);
    //sunModel.SetShaderTextureNamePrefix("material.");

    std::vector<Planet*> planets {
        &earth, &mars, &venus, &neptune
    };

    PointLight& pointLight = programState->pointLight;
    pointLight.position = programState->sunPosition;
    //pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.ambient = glm::vec3(1, 1, 1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;



    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    calculateCenterOfMass(planets);

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

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        ourShader.setVec3("pointLight.position", programState->sunPosition);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);

        if(firstLook){
            glm::mat4 startAtSun = glm::lookAt(
            programState->sunPosition - programState->camera.Position,
            programState->camera.Position,
            programState->camera.Up
            );

            firstLook = false;
            ourShader.setMat4("view", startAtSun);
        }
        else {
            glm::mat4 view = programState->camera.GetViewMatrix();
            ourShader.setMat4("view", view);
        }

        sunShader.setMat4("view", programState->camera.GetViewMatrix());

        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        sunShader.setMat4("projection", projection);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->sunPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.3));    // it's a bit too big for our scene, so scale it down
        sunShader.setMat4("model", model);
        ourShader.setMat4("model", model);
        sunModel.Draw(ourShader);

        pointLight.position = sunModel.getPosition();

        for(Planet *p : planets) {
            p->Draw(ourShader);
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
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("sun position", (float*)&programState->sunScale);
        ImGui::DragFloat("sun scale", &programState->sunScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
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
