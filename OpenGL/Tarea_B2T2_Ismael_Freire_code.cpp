#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_s.h>

#define STB_IMAGE_IMPLEMENTATION 
#include <learnopengl/stb_image.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "B2T2 - Ismael Freire - 1752906741", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("shaders/B2T2_vertex.vs", "shaders/B2T2_fragment.fs"); // you can name your shader files however you like

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    // Definición de Vértices con Coordenadas de Textura Calculadas
    // Formato: X, Y, Z,   R, G, B,   S, T
    float vertices[] = {
        // --- GRUPO 1 (limites: X[-0.7 a 0.0], Y[-0.6 a 0.5]) ---

        // 1. CABEZA (Azul)                                   //Indices 
        -0.5f,  0.5f, 0.0f,   0.2f, 0.6f, 1.0f, 0.29f, 1.00f, // A    0
        -0.7f,  0.3f, 0.0f,   0.2f, 0.6f, 1.0f, 0.00f, 0.82f, // B    1
        -0.3f,  0.3f, 0.0f,   0.2f, 0.6f, 1.0f, 0.57f, 0.82f, // C    2

        // 2. CUELLO (Rojo - Cuadrado)
        // Usamos 4 vértices en lugar de 6
        -0.6f,  0.3f, 0.0f,   1.0f, 0.3f, 0.3f, 0.14f, 0.82f, // D    3
        -0.3f,  0.3f, 0.0f,   1.0f, 0.3f, 0.3f, 0.57f, 0.82f, // C    4
        -0.6f,  0.0f, 0.0f,   1.0f, 0.3f, 0.3f, 0.14f, 0.55f, // E    5
        -0.3f,  0.0f, 0.0f,   1.0f, 0.3f, 0.3f, 0.57f, 0.55f, // F    6

        // 3. CUERPO (Verde)
        -0.6f,  0.0f, 0.0f,   0.2f, 0.8f, 0.2f, 0.14f, 0.55f, // E    7   
         0.0f,  0.0f, 0.0f,   0.2f, 0.8f, 0.2f, 1.00f, 0.55f, // H    8
         0.0f, -0.6f, 0.0f,   0.2f, 0.8f, 0.2f, 1.00f, 0.00f, // G    9

        // 4. PATA (Naranja)
        -0.4f, -0.6f, 0.0f,   1.0f, 0.6f, 0.2f, 0.43f, 0.00f, // J    10
        -0.2f, -0.4f, 0.0f,   1.0f, 0.6f, 0.2f, 0.71f, 0.18f, // I    11
         0.0f, -0.6f, 0.0f,   1.0f, 0.6f, 0.2f, 1.00f, 0.00f, // G    12

        // --- GRUPO 2 (limites: X[0.0 a 0.6], Y[-0.3 a 0.8]) ---

        // 5. COLA (Amarillo)
        0.0f,  0.3f, 0.0f,   1.0f, 0.8f, 0.1f, 0.00f, 0.70f, // K     13
        0.6f,  0.3f, 0.0f,   1.0f, 0.8f, 0.1f, 1.00f, 0.70f, // L     14
        0.0f,  -0.3f, 0.0f,  1.0f, 0.8f, 0.1f, 0.00f, 0.00f, // M    15
        
        // 6. COLA ALTA (Verde Claro)
        0.2f,  0.3f, 0.0f,   0.6f, 0.9f, 0.2f, 0.35f, 0.65f, // R     16
        0.6f,  0.3f, 0.0f,   0.6f, 0.9f, 0.2f, 1.00f, 0.65f, // L     17
        0.6f,  0.8f, 0.0f,   0.6f, 0.9f, 0.2f, 1.00f, 0.15f, // Q     18

        // 7. COLA BAJA (Morado - Paralelogramo)
        // Se usa 4 vértices en lugar de 6
        0.6f, 0.3f, 0.0f,   0.4f, 0.3f, 0.8f, 1.00f, 0.70f, // L     19
        0.3f, 0.0f, 0.0f,   0.4f, 0.3f, 0.8f, 0.58f, 0.33f, // N     20
        0.3f, -0.3f, 0.0f,  0.4f, 0.3f, 0.8f, 0.58f, 0.00f, // O     21
        0.6f, 0.0f, 0.0f,   0.4f, 0.3f, 0.8f, 1.00f, 0.33f  // P     22
    }; 
	
    // DIVISIÓN EN 2 GRUPOS 
    // INDICES (EBO) 
    // Definimos como conectar los vertices de arriba para formar la figura

    // Grupo 1
    unsigned int indicesGroup1[] = {
        0, 1, 2,    // 1. Cabeza

        3, 4, 6,    // 2. Cuello (Triangulo 1 del cuadrado)
        3, 5, 6,    // 2. Cuello (Triangulo 2 del cuadrado)

        7, 8, 9,    // 3. Cuerpo

        10, 11, 12, // 4. Pata
    };

	// Grupo 2
    unsigned int indicesGroup2[] = {
        13, 14, 15, // 5. Cola

        16, 17, 18, // 6. Cola Alta

        19, 20, 22, // 7. Cola baja
        20, 22, 21
	};


    unsigned int VBO, VAO;
	unsigned int EBO1, EBO2;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO1);
    glGenBuffers(1, &EBO2);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Cargar los indices al grupo1
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO1);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesGroup1), indicesGroup1, GL_STATIC_DRAW);

	// Cargar los indices al grupo2
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesGroup2), indicesGroup2, GL_STATIC_DRAW);

	// Configurar los atributos de los vértices
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // textura
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

	// Desvincular VBO y VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 5. CARGA DE TEXTURAS
    unsigned int texture1, texture2;

    // --- Textura 1 ---
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    // Configuración de wrapping/filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // Voltear imagen porque OpenGL tiene el eje Y invertido
    unsigned char* data = stbi_load("textures/texture1.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        // Generar textura dependiendo si tiene alpha o no
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture1" << std::endl;
    }
    stbi_image_free(data);

    // --- Textura 2 ---
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load("textures/texture2.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture2" << std::endl;
    }
    stbi_image_free(data);

    ourShader.setInt("ourTexture", 0); // Decimos al shader que use la textura unit 0

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // render the triangle
        ourShader.use();

        // --- TAREA B2T2 ---

        // 1. Obtener el tiempo
        float timeValue = glfwGetTime();

        // 2. Definir la Matriz Identidad
        glm::mat4 transform = glm::mat4(1.0f);

        // 3. TRASLACIÓN (Mover en círculos)
        // Se usa Sin y Cos para mover X y Y en un círculo de radio 0.5
        transform = glm::translate(transform, glm::vec3(0.3f * sin(timeValue), 0.3f * cos(timeValue), 0.0f));

        // 4. ROTACIÓN (Girar sobre su eje Z)
        // Girar en función del tiempo
        transform = glm::rotate(transform, timeValue, glm::vec3(0.0f, 0.0f, 1.0f));

        // 5. ESCALADO
        // El tamaño varía entre 0.8 y 1.2
        float scaleAmount = 1.0f + 0.2f * sin(timeValue * 3.0f);
        transform = glm::scale(transform, glm::vec3(scaleAmount, scaleAmount, 1.0f));

		// 6. Enviar la matriz al shader
        unsigned int transformLoc = glGetUniformLocation(ourShader.ID, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

        glBindVertexArray(VAO);
        
		// Dibujar el Grupo 1
        glActiveTexture(GL_TEXTURE0); 
        glBindTexture(GL_TEXTURE_2D, texture1); // Enlazamos la Textura 1

		// enlazar el EBO1
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO1);
		// Dibujar los elementos del Grupo 1
		glDrawElements(GL_TRIANGLES, 15, GL_UNSIGNED_INT, 0);

		// Dibujar el Grupo 2
        glActiveTexture(GL_TEXTURE0); 
        glBindTexture(GL_TEXTURE_2D, texture2); // Cambiamos a la Textura 2

		// enlazar el EBO2
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO1);
    glDeleteBuffers(1, &EBO2);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// -------------------------------------------------------------------------------------------------------- -
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}