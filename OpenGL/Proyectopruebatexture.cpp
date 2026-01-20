// ===================== LABERINTO DESDE TXT - SIN TEXTURAS =====================
// Requiere B2T3.fs con uniform vec3 baseColor (sin samplear texture1).

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// ================= CONFIG =================
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ================= ESCALA / RENDER =================
static const float TARGET_WORLD_WIDTH = 30.0f; // queremos que el mapa ~30u de ancho
float TILE = 2.5f;                             // se recalcula al cargar
const float WALL_HEIGHT = 1.6f;                 // paredes más bajas
const float LIGHT_CUBE_SCALE = 0.25f;

// ================= LUZ =================
glm::vec3 lightPos(0.0f, 6.0f, 0.0f);
bool linternaEncendida = false;

// ================= CAMARA =================
class Camera {
public:
    glm::vec3 Position{ 0.0f, 2.0f, 10.0f };
    glm::vec3 Front{ 0.0f, 0.0f, -1.0f };
    glm::vec3 Up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 Right;
    glm::vec3 WorldUp{ 0.0f, 1.0f, 0.0f };

    float Yaw{ -90.0f };
    float Pitch{ 0.0f };
    float Speed{ 6.0f };
    float Sensitivity{ 0.1f };
    float Zoom{ 45.0f };

    Camera() { update(); }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void Keyboard(int dir, float dt) {
        float v = Speed * dt;
        if (dir == 0) Position += Front * v;
        if (dir == 1) Position -= Front * v;
        if (dir == 2) Position -= Right * v;
        if (dir == 3) Position += Right * v;
    }

    void Mouse(float x, float y) {
        x *= Sensitivity;
        y *= Sensitivity;
        Yaw += x;
        Pitch = glm::clamp(Pitch + y, -89.0f, 89.0f);
        update();
    }

    void SetPose(const glm::vec3& pos, float yawDeg, float pitchDeg) {
        Position = pos;
        Yaw = yawDeg;
        Pitch = glm::clamp(pitchDeg, -89.0f, 89.0f);
        update();
    }

private:
    void update() {
        glm::vec3 f;
        f.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        f.y = sin(glm::radians(Pitch));
        f.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(f);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

Camera camera;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// ================= CALLBACKS =================
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    camera.Mouse((float)xpos - lastX, lastY - (float)ypos);
    lastX = (float)xpos;
    lastY = (float)ypos;
}

// ================= INPUT =================
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.Keyboard(0, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.Keyboard(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.Keyboard(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.Keyboard(3, deltaTime);

    // Linterna L (toggle)
    static bool lPrevState = false;
    bool lState = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
    if (lState && !lPrevState) linternaEncendida = !linternaEncendida;
    lPrevState = lState;
}

// ================= MAPA DESDE TXT =================
// 0 = vacío, 1 = piso/camino, E = salida, S = spawn
static std::vector<std::string> MAP;
static int MAP_W = 0;
static int MAP_H = 0;

static bool LoadMapFromTxt(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "No se pudo abrir: " << path << "\n";
        return false;
    }

    MAP.clear();
    std::string line;
    while (std::getline(file, line)) {
        // limpiar CR si viene de Windows (line endings)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        MAP.push_back(line);
    }

    if (MAP.empty()) {
        std::cerr << "Mapa vacio.\n";
        return false;
    }

    MAP_H = (int)MAP.size();
    MAP_W = (int)MAP[0].size();

    // Validar ancho uniforme
    for (int r = 0; r < MAP_H; r++) {
        if ((int)MAP[r].size() != MAP_W) {
            std::cerr << "Fila " << r << " tiene ancho diferente.\n";
            return false;
        }
    }

    // TILE auto: el mapa siempre ~TARGET_WORLD_WIDTH de ancho
    TILE = TARGET_WORLD_WIDTH / (float)MAP_W;

    std::cout << "Mapa cargado OK: " << MAP_W << "x" << MAP_H << " | TILE=" << TILE << "\n";
    return true;
}

static inline bool InBounds(int r, int c) {
    return r >= 0 && c >= 0 && r < MAP_H && c < MAP_W;
}

static inline char Cell(int r, int c) {
    if (!InBounds(r, c)) return '0';
    return MAP[r][c];
}

static inline bool Walkable(int r, int c) {
    char t = Cell(r, c);
    return t == '1' || t == 'E' || t == 'S';
}

static inline bool IsExit(int r, int c) { return Cell(r, c) == 'E'; }
static inline bool IsSpawn(int r, int c) { return Cell(r, c) == 'S'; }

static bool FindSpawn(int& outR, int& outC) {
    // buscar 'S'
    for (int r = 0; r < MAP_H; r++) {
        for (int c = 0; c < MAP_W; c++) {
            if (IsSpawn(r, c)) { outR = r; outC = c; return true; }
        }
    }
    // fallback: primera '1'
    for (int r = 0; r < MAP_H; r++) {
        for (int c = 0; c < MAP_W; c++) {
            if (Cell(r, c) == '1') { outR = r; outC = c; return true; }
        }
    }
    return false;
}

static glm::vec3 CellToWorld(int r, int c) {
    float halfW = (MAP_W * TILE) * 0.5f;
    float halfH = (MAP_H * TILE) * 0.5f;

    float x = (c * TILE) - halfW;
    float z = (halfH)-(r * TILE);
    return glm::vec3(x, 0.0f, z);
}

// ================= MAIN =================
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LABERINTO (sin texturas) - TXT", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Error al inicializar GLAD\n";
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    Shader shader("shaders/B2T3.vs", "shaders/B2T3.fs");
    Shader lightShader("shaders/light_cube.vs", "shaders/light_cube.fs");

    // ===== CARGAR MAPA DESDE TXT =====
    // Coloca maze.txt en la misma carpeta donde corre el .exe (working directory).
    if (!LoadMapFromTxt("maze.txt")) {
        std::cerr << "No se pudo cargar maze.txt\n";
        return -1;
    }

    int sr = 0, sc = 0;
    if (!FindSpawn(sr, sc)) {
        std::cerr << "No hay 'S' ni '1' en el mapa.\n";
        return -1;
    }

    // Luz al centro del mapa
    {
        glm::vec3 center = CellToWorld(MAP_H / 2, MAP_W / 2);
        lightPos = glm::vec3(center.x, 6.0f, center.z);
    }

    // Cámara en spawn (un poco hacia atrás en Z para que vea algo)
    {
        glm::vec3 spawnW = CellToWorld(sr, sc);
        camera.SetPose(glm::vec3(spawnW.x, 2.0f, spawnW.z + 2.0f), -90.0f, 0.0f);
    }

    // ===== GEOMETRIA BASE =====
    float planeVertices[] = {
        -0.5f,0,-0.5f,  0,1,0,   0,0,
         0.5f,0,-0.5f,  0,1,0,   1,0,
         0.5f,0, 0.5f,  0,1,0,   1,1,
         0.5f,0, 0.5f,  0,1,0,   1,1,
        -0.5f,0, 0.5f,  0,1,0,   0,1,
        -0.5f,0,-0.5f,  0,1,0,   0,0
    };

    float wallVertices[] = {
        -0.5f,0,0,  0,0,1,  0,1,
         0.5f,0,0,  0,0,1,  1,1,
         0.5f,1,0,  0,0,1,  1,0,
         0.5f,1,0,  0,0,1,  1,0,
        -0.5f,1,0,  0,0,1,  0,0,
        -0.5f,0,0,  0,0,1,  0,1
    };

    // Cubo para la luz
    float cubeVertices[] = {
        -0.5f,-0.5f,-0.5f,   0,0,-1, 0,0,
         0.5f, 0.5f,-0.5f,   0,0,-1, 1,1,
         0.5f,-0.5f,-0.5f,   0,0,-1, 1,0,
         0.5f, 0.5f,-0.5f,   0,0,-1, 1,1,
        -0.5f,-0.5f,-0.5f,   0,0,-1, 0,0,
        -0.5f, 0.5f,-0.5f,   0,0,-1, 0,1,

        -0.5f,-0.5f, 0.5f,   0,0, 1, 0,0,
         0.5f,-0.5f, 0.5f,   0,0, 1, 1,0,
         0.5f, 0.5f, 0.5f,   0,0, 1, 1,1,
         0.5f, 0.5f, 0.5f,   0,0, 1, 1,1,
        -0.5f, 0.5f, 0.5f,   0,0, 1, 0,1,
        -0.5f,-0.5f, 0.5f,   0,0, 1, 0,0,

        -0.5f, 0.5f, 0.5f,  -1,0,0, 1,0,
        -0.5f, 0.5f,-0.5f,  -1,0,0, 1,1,
        -0.5f,-0.5f,-0.5f,  -1,0,0, 0,1,
        -0.5f,-0.5f,-0.5f,  -1,0,0, 0,1,
        -0.5f,-0.5f, 0.5f,  -1,0,0, 0,0,
        -0.5f, 0.5f, 0.5f,  -1,0,0, 1,0,

         0.5f, 0.5f, 0.5f,   1,0,0, 1,0,
         0.5f,-0.5f,-0.5f,   1,0,0, 0,1,
         0.5f, 0.5f,-0.5f,   1,0,0, 1,1,
         0.5f,-0.5f,-0.5f,   1,0,0, 0,1,
         0.5f, 0.5f, 0.5f,   1,0,0, 1,0,
         0.5f,-0.5f, 0.5f,   1,0,0, 0,0,

        -0.5f,-0.5f,-0.5f,   0,-1,0, 0,1,
         0.5f,-0.5f,-0.5f,   0,-1,0, 1,1,
         0.5f,-0.5f, 0.5f,   0,-1,0, 1,0,
         0.5f,-0.5f, 0.5f,   0,-1,0, 1,0,
        -0.5f,-0.5f, 0.5f,   0,-1,0, 0,0,
        -0.5f,-0.5f,-0.5f,   0,-1,0, 0,1,

        -0.5f, 0.5f,-0.5f,   0, 1,0, 0,1,
         0.5f, 0.5f,-0.5f,   0, 1,0, 1,1,
         0.5f, 0.5f, 0.5f,   0, 1,0, 1,0,
         0.5f, 0.5f, 0.5f,   0, 1,0, 1,0,
        -0.5f, 0.5f, 0.5f,   0, 1,0, 0,0,
        -0.5f, 0.5f,-0.5f,   0, 1,0, 0,1
    };

    unsigned int floorVAO, floorVBO, wallVAO, wallVBO, cubeVAO, cubeVBO;

    // Piso
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    // Pared
    glGenVertexArrays(1, &wallVAO);
    glGenBuffers(1, &wallVBO);
    glBindVertexArray(wallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    // Cubo luz
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    // ================= LOOP =================
    while (!glfwWindowShouldClose(window)) {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;

        processInput(window);

        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 250.0f);
        glm::mat4 view = camera.GetViewMatrix();

        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("viewPos", camera.Position);

        shader.setInt("linterna", linternaEncendida);
        if (linternaEncendida) {
            shader.setVec3("spotLightPos", camera.Position);
            shader.setVec3("spotLightDir", camera.Front);
            shader.setFloat("spotCutOff", glm::cos(glm::radians(15.0f)));
            shader.setFloat("spotOuterCutOff", glm::cos(glm::radians(25.0f)));
        }
        else {
            shader.setVec3("spotLightPos", glm::vec3(0));
            shader.setVec3("spotLightDir", glm::vec3(0));
            shader.setFloat("spotCutOff", 0.0f);
            shader.setFloat("spotOuterCutOff", 0.0f);
        }

        // ===== DIBUJAR SUELO + PAREDES =====
        for (int r = 0; r < MAP_H; r++) {
            for (int c = 0; c < MAP_W; c++) {
                if (!Walkable(r, c)) continue;

                glm::vec3 w = CellToWorld(r, c);

                // ----- Suelo -----
                glBindVertexArray(floorVAO);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(w.x, 0.0f, w.z));
                model = glm::scale(model, glm::vec3(TILE, 1.0f, TILE));
                shader.setMat4("model", model);

                // colores sólidos (solo referencia)
                shader.setVec3("baseColor", IsExit(r, c) ? glm::vec3(0.55f, 0.20f, 0.65f) : glm::vec3(0.78f, 0.65f, 0.20f));
                glDrawArrays(GL_TRIANGLES, 0, 6);

                // ----- Paredes por borde -----
                glBindVertexArray(wallVAO);
                shader.setVec3("baseColor", glm::vec3(0.90f, 0.90f, 0.90f));

                // Norte (r-1,c)
                if (!Walkable(r - 1, c)) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x, 0.0f, w.z + TILE * 0.5f));
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }

                // Sur (r+1,c)
                if (!Walkable(r + 1, c)) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x, 0.0f, w.z - TILE * 0.5f));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }

                // Oeste (r,c-1)
                if (!Walkable(r, c - 1)) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x - TILE * 0.5f, 0.0f, w.z));
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0, 1, 0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }

                // Este (r,c+1)
                if (!Walkable(r, c + 1)) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x + TILE * 0.5f, 0.0f, w.z));
                    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
        }

        // ===== CUBO LUZ =====
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);

        glm::mat4 mLight = glm::mat4(1.0f);
        mLight = glm::translate(mLight, lightPos);
        mLight = glm::scale(mLight, glm::vec3(LIGHT_CUBE_SCALE));
        lightShader.setMat4("model", mLight);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
