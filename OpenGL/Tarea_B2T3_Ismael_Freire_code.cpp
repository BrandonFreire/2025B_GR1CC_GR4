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

//camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f)); 
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int main()
{
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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tarea B2T3 - Ismael Freire - 1752906741", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //Exercise 12 Task 3
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);


    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("shaders/B2T3_vertex.vs", "shaders/B2T3_fragment.fs");

    // Tarea 1
    float vertices[] = {
        // positions          // texture coords
        // CARA TRASERA 
        -0.5f, -0.5f, -0.5f,  0.180f, 0.508f,
         0.5f, -0.5f, -0.5f,  0.363f, 0.508f,
         0.5f,  0.5f, -0.5f,  0.363f, 0.672f,
         0.5f,  0.5f, -0.5f,  0.363f, 0.672f,
        -0.5f,  0.5f, -0.5f,  0.180f, 0.672f,
        -0.5f, -0.5f, -0.5f,  0.180f, 0.508f,

        // CARA DELANTERA 
        -0.5f, -0.5f,  0.5f,  0.545f, 0.507f,
         0.5f, -0.5f,  0.5f,  0.726f, 0.507f,
         0.5f,  0.5f,  0.5f,  0.726f, 0.670f,
         0.5f,  0.5f,  0.5f,  0.726f, 0.670f,
        -0.5f,  0.5f,  0.5f,  0.545f, 0.670f,
        -0.5f, -0.5f,  0.5f,  0.545f, 0.507f,
        
        //  CARA IZQUIERDA 
        -0.5f,  0.5f,  0.5f,  0.545f, 0.672f,
        -0.5f,  0.5f, -0.5f,  0.364f, 0.672f,
        -0.5f, -0.5f, -0.5f,  0.364f, 0.507f,
        -0.5f, -0.5f, -0.5f,  0.364f, 0.507f,
        -0.5f, -0.5f,  0.5f,  0.545f, 0.507f,
        -0.5f,  0.5f,  0.5f,  0.545f, 0.672f,

        // CARA DERECHA 
         0.5f,  0.5f,  0.5f,  0.729f, 0.671f,
         0.5f,  0.5f, -0.5f,  0.908f, 0.672f,
         0.5f, -0.5f, -0.5f,  0.908f, 0.506f,
         0.5f, -0.5f, -0.5f,  0.908f, 0.506f,
         0.5f, -0.5f,  0.5f,  0.727f, 0.507f,
         0.5f,  0.5f,  0.5f,  0.729f, 0.671f,

        // CARA INFERIOR 
        -0.5f, -0.5f, -0.5f,  0.546f, 0.271f,
         0.5f, -0.5f, -0.5f,  0.729f, 0.271f,
         0.5f, -0.5f,  0.5f,  0.729f, 0.506f,
         0.5f, -0.5f,  0.5f,  0.729f, 0.506f,
        -0.5f, -0.5f,  0.5f,  0.546f, 0.506f,
        -0.5f, -0.5f, -0.5f,  0.546f, 0.271f,

        // CARA SUPERIOR
        -0.5f,  0.5f, -0.5f,  0.545f, 0.908f,
         0.5f,  0.5f, -0.5f,  0.730f, 0.908f,
         0.5f,  0.5f,  0.5f,  0.730f, 0.673f,
         0.5f,  0.5f,  0.5f,  0.730f, 0.673f,
        -0.5f,  0.5f,  0.5f,  0.545f, 0.673f,
        -0.5f,  0.5f, -0.5f,  0.545f, 0.908f
    };

    float floorVertices[] = {
		// POSICIONES (X, Y, Z)       // TEXTURAS (S, T)
        // Cara Trasera
        -0.5f, -0.5f, -0.5f,  0.7233f, 0.3280f,
         0.5f, -0.5f, -0.5f,  0.9333f, 0.3280f,
         0.5f,  0.5f, -0.5f,  0.9333f, 0.5840f,
         0.5f,  0.5f, -0.5f,  0.9333f, 0.5840f,
        -0.5f,  0.5f, -0.5f,  0.7233f, 0.5840f,
        -0.5f, -0.5f, -0.5f,  0.7233f, 0.3280f,

        // Cara Delantera
        -0.5f, -0.5f,  0.5f,  0.295f, 0.328f,
         0.5f, -0.5f,  0.5f,  0.508f, 0.328f,
         0.5f,  0.5f,  0.5f,  0.508f, 0.584f,
         0.5f,  0.5f,  0.5f,  0.508f, 0.584f,
        -0.5f,  0.5f,  0.5f,  0.295f, 0.584f,
        -0.5f, -0.5f,  0.5f,  0.295f, 0.328f,

        // Cara Izquierda
        -0.5f,  0.5f,  0.5f,  0.295f, 0.328f,
        -0.5f,  0.5f, -0.5f,  0.508f, 0.328f,
        -0.5f, -0.5f, -0.5f,  0.508f, 0.584f,
        -0.5f, -0.5f, -0.5f,  0.508f, 0.584f,
        -0.5f, -0.5f,  0.5f,  0.295f, 0.584f,
        -0.5f,  0.5f,  0.5f,  0.295f, 0.328f,

        // Cara Derecha
         0.5f,  0.5f,  0.5f,  0.5100f, 0.5840f,
         0.5f,  0.5f, -0.5f,  0.7217f, 0.5840f,
         0.5f, -0.5f, -0.5f,  0.7217f, 0.3280f,
         0.5f, -0.5f, -0.5f,  0.7217f, 0.3280f,
         0.5f, -0.5f,  0.5f,  0.5100f, 0.3280f,
         0.5f,  0.5f,  0.5f,  0.5100f, 0.5840f,

         // Cara Inferior 
         -0.5f, -0.5f, -0.5f,  0.295f, 0.326f,
          0.5f, -0.5f, -0.5f,  0.508f, 0.326f,
          0.5f, -0.5f,  0.5f,  0.508f, 0.073f,
          0.5f, -0.5f,  0.5f,  0.508f, 0.073f,
         -0.5f, -0.5f,  0.5f,  0.295f, 0.073f,
         -0.5f, -0.5f, -0.5f,  0.295f, 0.326f,

         // Cara Superior  
         -0.5f,  0.5f, -0.5f,  0.296f, 0.839f,
          0.5f,  0.5f, -0.5f,  0.507f, 0.840f,
          0.5f,  0.5f,  0.5f,  0.508f, 0.586f,
          0.5f,  0.5f,  0.5f,  0.508f, 0.586f,
         -0.5f,  0.5f,  0.5f,  0.295f, 0.588f,
         -0.5f,  0.5f, -0.5f,  0.296f, 0.839f
    };

    // Tarea 2
    float verticesChar2[]{
        // POSICIONES (X, Y, Z)       // TEXTURAS (S, T)
        // Cara Trasera
        -0.5f, -0.5f, -0.5f,  0.7027f, 0.6625f,
         0.5f, -0.5f, -0.5f,  0.8973f, 0.6625f,
         0.5f,  0.5f, -0.5f,  0.8973f, 0.8010f,
         0.5f,  0.5f, -0.5f,  0.8973f, 0.8010f,
        -0.5f,  0.5f, -0.5f,  0.7027f, 0.8010f,
        -0.5f, -0.5f, -0.5f,  0.7027f, 0.6625f,

        // Cara Delantera
        -0.5f, -0.5f,  0.5f,  0.3149f, 0.6635f,
         0.5f, -0.5f,  0.5f,  0.5081f, 0.6635f,
         0.5f,  0.5f,  0.5f,  0.5081f, 0.8010f,
         0.5f,  0.5f,  0.5f,  0.5081f, 0.8010f,
        -0.5f,  0.5f,  0.5f,  0.3149f, 0.8010f,
        -0.5f, -0.5f,  0.5f,  0.3149f, 0.6635f,

        // Cara Izquierda
        -0.5f,  0.5f,  0.5f,  0.3121f, 0.8009f,
        -0.5f,  0.5f, -0.5f,  0.1202f, 0.8009f,
        -0.5f, -0.5f, -0.5f,  0.1202f, 0.6625f,
        -0.5f, -0.5f, -0.5f,  0.1202f, 0.6625f,
        -0.5f, -0.5f,  0.5f,  0.3121f, 0.6625f,
        -0.5f,  0.5f,  0.5f,  0.3121f, 0.8009f,

        // Cara Derecha
         0.5f,  0.5f,  0.5f,  0.5094f, 0.8010f,
         0.5f,  0.5f, -0.5f,  0.7027f, 0.8010f,
         0.5f, -0.5f, -0.5f,  0.7027f, 0.6625f,
         0.5f, -0.5f, -0.5f,  0.7027f, 0.6625f,
         0.5f, -0.5f,  0.5f,  0.5094f, 0.6625f,
         0.5f,  0.5f,  0.5f,  0.5094f, 0.8010f,

         // Cara Inferior 
         -0.5f, -0.5f, -0.5f,  0.3149f, 0.5240f,
          0.5f, -0.5f, -0.5f,  0.5081f, 0.5240f,
          0.5f, -0.5f,  0.5f,  0.5081f, 0.6615f,
          0.5f, -0.5f,  0.5f,  0.5081f, 0.6615f,
         -0.5f, -0.5f,  0.5f,  0.3149f, 0.6615f,
         -0.5f, -0.5f, -0.5f,  0.3149f, 0.5240f,

         // Cara Superior  
         -0.5f,  0.5f, -0.5f,  0.3149f, 0.9385f,
          0.5f,  0.5f, -0.5f,  0.5081f, 0.9385f,
          0.5f,  0.5f,  0.5f,  0.5081f, 0.8010f,
          0.5f,  0.5f,  0.5f,  0.5081f, 0.8010f,
         -0.5f,  0.5f,  0.5f,  0.3149f, 0.8010f,
         -0.5f,  0.5f, -0.5f,  0.3149f, 0.9385f

    };


    // Declarar variables 
    unsigned int VAO_Char, VBO_Char;
    unsigned int VAO_Char2, VBO_Char2;
    unsigned int VAO_Floor, VBO_Floor;

    // Configuracion para el personaje 1
    glGenVertexArrays(1, &VAO_Char);
    glGenBuffers(1, &VBO_Char);
    glBindVertexArray(VAO_Char);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Char);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Atributos del personaje
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Configuracion del suelo
    glGenVertexArrays(1, &VAO_Floor);
    glGenBuffers(1, &VBO_Floor);
    glBindVertexArray(VAO_Floor);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Floor);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

    // Atributos del suelo
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // Configuracion del personaje 2
    glGenVertexArrays(1, &VAO_Char2);
    glGenBuffers(1, &VBO_Char2);
    glBindVertexArray(VAO_Char2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Char2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesChar2), verticesChar2, GL_STATIC_DRAW);

    // Atributos del personaje
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Desvincular
    glBindVertexArray(0);


    // load and create a texture 
    // -------------------------
    unsigned int textureFloor, textureModel, textureModel2;
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    // texture 1 (suelo)
    // ---------
    glGenTextures(1, &textureFloor);
    glBindTexture(GL_TEXTURE_2D, textureFloor);
    unsigned char* data = stbi_load("textures/texture_floor.png", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    // texture 2 (personaje 1)
    // ---------
    glGenTextures(1, &textureModel);
    glBindTexture(GL_TEXTURE_2D, textureModel);
    data = stbi_load("textures/texture_model.png", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    // texture 2 (personaje 2)
    // ---------
    glGenTextures(1, &textureModel2);
    glBindTexture(GL_TEXTURE_2D, textureModel2);
    data = stbi_load("textures/texture_model_2.png", &width, &height, &nrChannels, 4);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);


    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    ourShader.use();
    ourShader.setInt("texture1", 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // Matrices de Cámara 
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // 1. DIBUJAR EL SUELO (TAREA 1) 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureFloor);
        glBindVertexArray(VAO_Floor);
        for (int x = -10; x < 10; x++) {
            for (int z = -10; z < 10; z++) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3((float)x, -1.0f, (float)z));
                ourShader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // 2. Dibujar al personaje 1 (TAREA 1) 
        glBindTexture(GL_TEXTURE_2D, textureModel);
        glBindVertexArray(VAO_Char);
        glm::mat4 modelChar = glm::mat4(1.0f);
        modelChar = glm::translate(modelChar, glm::vec3(0.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", modelChar);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Clon del personaje 1
        glm::mat4 modelChar1Clone = glm::mat4(1.0f);
        modelChar1Clone = glm::translate(modelChar1Clone, glm::vec3(-3.0f, 0.5f, -2.0f)); // Traslación
        modelChar1Clone = glm::rotate(modelChar1Clone, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación 
        modelChar1Clone = glm::scale(modelChar1Clone, glm::vec3(0.5f, 0.5f, 0.5f)); // Escalado 
        ourShader.setMat4("model", modelChar1Clone);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 3. Dibujar al personaje 2 (TAREA 2) 
        glBindTexture(GL_TEXTURE_2D, textureModel2); 
        glBindVertexArray(VAO_Char2);
        glm::mat4 modelChar2 = glm::mat4(1.0f);
        modelChar2 = glm::translate(modelChar2, glm::vec3(2.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", modelChar2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

		// Clon del personaje 2
        glm::mat4 modelChar2Clone = glm::mat4(1.0f);
        modelChar2Clone = glm::translate(modelChar2Clone, glm::vec3(5.0f, 1.0f, -3.0f)); // Traslación
        modelChar2Clone = glm::rotate(modelChar2Clone, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotación 
        modelChar2Clone = glm::scale(modelChar2Clone, glm::vec3(1.5f, 1.5f, 1.5f)); // Escalado
        ourShader.setMat4("model", modelChar2Clone);
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
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
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

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
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