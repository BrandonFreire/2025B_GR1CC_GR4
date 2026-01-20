#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// TAREA
// 1. Se modifica el Vertex Shader para que acepte el color como atributo (location = 1)
//    y lo pase al Fragment Shader (usando 'out vec3 ourColor')
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n" // Nuevo atributo de color
"out vec3 ourColor;\n"                     // Variable de salida para el frag shader
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos, 1.0);\n"
"   ourColor = aColor;\n"                  // Pasa el color
"}\0";

// 2. Se modifica el Fragment Shader para que reciba el color 
//    y lo use para 'FragColor'
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 ourColor;\n" // Variable de entrada desde el vertex shader
"void main()\n"
"{\n"
"   FragColor = vec4(ourColor, 1.0f);\n" // Usa el color del vértice
"}\n\0";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "First Triangle", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Nombre y cédula en la ventana
    glfwSetWindowTitle(window, "B1T3 - Ismael Freire - 1752906741");

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


	// Modificacion de los vertices y atributos
    // Se reemplaza el triángulo por el hexagono (6 triangulos, 18 vertices)
    // Cada vertice ahora tiene 6 floats: X, Y, Z, R, G, B
    float vertices[] = {
        // Posición (X, Y, Z)      // Color (R, G, B)
        // Triángulo 1 (Rojo)
       0.0f,  0.0f, 0.0f,   1.0f, 0.0f, 0.0f, // Centro
       0.8f,  0.0f, 0.0f,   1.0f, 0.0f, 0.0f, // Vértice 0
       0.4f,  0.7f, 0.0f,   1.0f, 0.0f, 0.0f, // Vértice 1

       // Triángulo 2 (Verde)
        0.0f,  0.0f, 0.0f,   0.0f, 1.0f, 0.0f, // Centro
        0.4f,  0.7f, 0.0f,   0.0f, 1.0f, 0.0f, // Vértice 1
        -0.4f, 0.7f, 0.0f,   0.0f, 1.0f, 0.0f, // Vértice 2

      // Triángulo 3 (Azul)
        0.0f,  0.0f, 0.0f,   0.0f, 0.0f, 1.0f, // Centro
        -0.4f,  0.7f, 0.0f,  0.0f, 0.0f, 1.0f, // Vértice 2
        -0.8f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f, // Vértice 3

     // Triángulo 4 (Amarillo)
        0.0f,  0.0f, 0.0f,   1.0f, 1.0f, 0.0f, // Centro
        -0.8f,  0.0f, 0.0f,   1.0f, 1.0f, 0.0f, // Vértice 3
        -0.4f, -0.7f, 0.0f,   1.0f, 1.0f, 0.0f, // Vértice 4

         // Triángulo 5 (Magenta)
        0.0f,  0.0f, 0.0f,   1.0f, 0.0f, 1.0f, // Centro
        -0.4f, -0.7f, 0.0f,  1.0f, 0.0f, 1.0f, // Vértice 4
        0.4f, -0.7f, 0.0f,   1.0f, 0.0f, 1.0f, // Vértice 5

   // Triángulo 6 (Cian)
        0.0f,  0.0f, 0.0f,   0.0f, 1.0f, 1.0f, // Centro
        0.4f, -0.7f, 0.0f,   0.0f, 1.0f, 1.0f, // Vértice 5
        0.8f,  0.0f, 0.0f,   0.0f, 1.0f, 1.0f  // Vértice 0
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    // Ahora se tiene 6 floats por vértice.
    // El stride total para un vértice es '6 * sizeof(float)'

    // 1. Atributo de Posición (location = 0)
    //    Sigue teniendo 3 floats, pero el stride es 6. El offset es 0.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 2. Atributo de Color (location = 1)
    //    Tiene 3 floats, el stride es 6.
    //    El offset es '3 * sizeof(float)' para saltar los 3 floats de la posicion
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Color de fondo
        glClear(GL_COLOR_BUFFER_BIT);

        // Dibujar el hexágono
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // Se dibuja 18 vértices (6 triángulos * 3 vértices) en lugar de 3
        glDrawArrays(GL_TRIANGLES, 0, 18);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}