#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>

#define STB_IMAGE_IMPLEMENTATION 
#include <learnopengl/stb_image.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tarea B2T4 - The Phong Illumination Model", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader lightingShader("shaders/B2T4_vertex.vs", "shaders/B2T4_fragment.fs");
    Shader lightCubeShader("shaders/light_cube.vs", "shaders/light_cube.fs");

    // Tarea 1: 
    float vertices[] = {
        // CARA TRASERA
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.180f, 0.508f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.363f, 0.508f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.363f, 0.672f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.363f, 0.672f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.180f, 0.672f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.180f, 0.508f,
        // CARA DELANTERA
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.545f, 0.507f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.726f, 0.507f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.726f, 0.670f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.726f, 0.670f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.545f, 0.670f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.545f, 0.507f,
        //  CARA IZQUIERDA
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.545f, 0.672f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.364f, 0.672f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.364f, 0.507f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.364f, 0.507f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.545f, 0.507f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.545f, 0.672f,
        // CARA DERECHA
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.729f, 0.671f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.908f, 0.672f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.908f, 0.506f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.908f, 0.506f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.727f, 0.507f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.729f, 0.671f,
         // CARA INFERIOR
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.546f, 0.271f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.729f, 0.271f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.729f, 0.506f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.729f, 0.506f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.546f, 0.506f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.546f, 0.271f,
        // CARA SUPERIOR
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.545f, 0.908f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.730f, 0.908f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.730f, 0.673f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.730f, 0.673f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.545f, 0.673f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.545f, 0.908f
    };

    // Suelo
    float floorVertices[] = {
        // CARA TRASERA
        // Pos                  // Normales           // UVs
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.7233f, 0.3280f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.9333f, 0.3280f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.9333f, 0.5840f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.9333f, 0.5840f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.7233f, 0.5840f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.7233f, 0.3280f,
        // CARA DELANTERA
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.295f, 0.328f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.508f, 0.328f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.508f, 0.584f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.508f, 0.584f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.295f, 0.584f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.295f, 0.328f,
        //  CARA IZQUIERDA
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.295f, 0.328f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.508f, 0.328f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.508f, 0.584f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.508f, 0.584f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.295f, 0.584f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.295f, 0.328f,
        // CARA DERECHA
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5100f, 0.5840f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.7217f, 0.5840f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.7217f, 0.3280f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.7217f, 0.3280f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5100f, 0.3280f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5100f, 0.5840f,
         // CARA INFERIOR
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.295f, 0.326f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.508f, 0.326f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.508f, 0.073f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.508f, 0.073f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.295f, 0.073f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.295f, 0.326f,
        // CARA SUPERIOR
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.296f, 0.839f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.507f, 0.840f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.508f, 0.586f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.508f, 0.586f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.295f, 0.588f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.296f, 0.839f
    };

    // Personaje 2
    float verticesChar2[]{
        // CARA TRASERA
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.7027f, 0.6625f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.8973f, 0.6625f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.8973f, 0.8010f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.8973f, 0.8010f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.7027f, 0.8010f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.7027f, 0.6625f,
        // CARA DELANTERA
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.3149f, 0.6635f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.5081f, 0.6635f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.5081f, 0.8010f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.5081f, 0.8010f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.3149f, 0.8010f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.3149f, 0.6635f,
        //  CARA IZQUIERDA
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.3121f, 0.8009f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.1202f, 0.8009f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.1202f, 0.6625f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.1202f, 0.6625f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.3121f, 0.6625f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.3121f, 0.8009f,
        // CARA DERECHA
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5094f, 0.8010f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.7027f, 0.8010f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.7027f, 0.6625f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.7027f, 0.6625f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5094f, 0.6625f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5094f, 0.8010f,
         // CARA INFERIOR
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.3149f, 0.5240f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.5081f, 0.5240f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.5081f, 0.6615f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.5081f, 0.6615f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.3149f, 0.6615f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.3149f, 0.5240f,
        // CARA SUPERIOR
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.3149f, 0.9385f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.5081f, 0.9385f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.5081f, 0.8010f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.5081f, 0.8010f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.3149f, 0.8010f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.3149f, 0.9385f
    };


    // Declarar variables 
    unsigned int VAO_Char, VBO_Char;
    unsigned int VAO_Char2, VBO_Char2;
    unsigned int VAO_Floor, VBO_Floor;
    unsigned int lightCubeVAO;

    // Configuracion para el personaje 1
    glGenVertexArrays(1, &VAO_Char);
    glGenBuffers(1, &VBO_Char);
    glBindVertexArray(VAO_Char);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Char);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Atributos del personaje
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Configuracion del suelo
    glGenVertexArrays(1, &VAO_Floor);
    glGenBuffers(1, &VBO_Floor);
    glBindVertexArray(VAO_Floor);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Floor);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

    // Atributos del suelo
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Configuracion del personaje 2
    glGenVertexArrays(1, &VAO_Char2);
    glGenBuffers(1, &VBO_Char2);
    glBindVertexArray(VAO_Char2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Char2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesChar2), verticesChar2, GL_STATIC_DRAW);

    // Atributos del personaje
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Configuracion de la LUZ 
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Char); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Texturas
    unsigned int textureFloor, textureModel, textureModel2;
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);

    glGenTextures(1, &textureFloor);
    glBindTexture(GL_TEXTURE_2D, textureFloor);
    unsigned char* data = stbi_load("textures/texture_floor.png", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    glGenTextures(1, &textureModel);
    glBindTexture(GL_TEXTURE_2D, textureModel);
    data = stbi_load("textures/texture_model.png", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    glGenTextures(1, &textureModel2);
    glBindTexture(GL_TEXTURE_2D, textureModel2);
    data = stbi_load("textures/texture_model_2.png", &width, &height, &nrChannels, 4);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);

    // RENDER LOOP
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // Posicion de la luz
        float lightX = 2.0f + sin(currentFrame) * 3.0f;
        float lightZ = cos(currentFrame) * 3.0f;
        glm::vec3 dynamicLightPos = glm::vec3(lightX, 1.5f, lightZ);

        // Dibujar Foco
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, dynamicLightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        lightCubeShader.setMat4("model", model);
        glBindVertexArray(lightCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Configurar Shader Principal
        lightingShader.use();
        lightingShader.setVec3("light.position", dynamicLightPos);
        lightingShader.setVec3("viewPos", camera.Position);

        // Intensidad de la luz
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        lightingShader.setVec3("light.ambient", lightColor * 0.2f);
        lightingShader.setVec3("light.diffuse", lightColor * 0.8f);
        lightingShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("material.shininess", 32.0f);
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // 1. SUELO
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureFloor);
        glBindVertexArray(VAO_Floor);
        for (int x = -10; x < 10; x++) {
            for (int z = -10; z < 10; z++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3((float)x, -1.0f, (float)z));
                lightingShader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // 2. PERSONAJE 1
        glBindTexture(GL_TEXTURE_2D, textureModel);
        glBindVertexArray(VAO_Char);
        model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Clon del personaje 1
        glm::mat4 modelChar1Clone = glm::mat4(1.0f);
        modelChar1Clone = glm::translate(modelChar1Clone, glm::vec3(-3.0f, 0.5f, -2.0f)); // Traslación
        modelChar1Clone = glm::rotate(modelChar1Clone, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación 
        modelChar1Clone = glm::scale(modelChar1Clone, glm::vec3(0.5f, 0.5f, 0.5f)); // Escalado 
        lightingShader.setMat4("model", modelChar1Clone);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 3. PERSONAJE 2
        glBindTexture(GL_TEXTURE_2D, textureModel2);
        glBindVertexArray(VAO_Char2);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
        lightingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Clon del personaje 2
        glm::mat4 modelChar2Clone = glm::mat4(1.0f);
        modelChar2Clone = glm::translate(modelChar2Clone, glm::vec3(5.0f, 1.0f, -4.0f)); // Traslación
        modelChar2Clone = glm::rotate(modelChar2Clone, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotación 
        modelChar2Clone = glm::scale(modelChar2Clone, glm::vec3(1.5f, 1.5f, 1.5f)); // Escalado
        lightingShader.setMat4("model", modelChar2Clone);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO_Char);
    glDeleteBuffers(1, &VBO_Char);
    glDeleteVertexArrays(1, &VAO_Char2);
    glDeleteBuffers(1, &VBO_Char2);
    glDeleteVertexArrays(1, &VAO_Floor);
    glDeleteBuffers(1, &VBO_Floor);
    glDeleteVertexArrays(1, &lightCubeVAO);

    glfwTerminate();
    return 0;
}
// Las funciones de callback (processInput, mouse_callback, etc) siguen igual abajo...
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}