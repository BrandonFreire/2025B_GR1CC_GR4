// ===================== LABERINTO DESDE TXT - SIN TEXTURAS =====================
// Lee un archivo maze.txt (0 = vacío, 1 = suelo, E = salida)
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
// prototipos colisión (para que processInput los vea)
static inline void MoveWithCollision(const glm::vec3& deltaXZ);

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
const float CELL_EPS = 1e-4f;   // epsilon contra errores de float


// ================= ESCALA =================
// Se calcula MAP_W/MAP_H desde el TXT. TILE mantiene tamaño similar a tu ROOM_SIZE (~30).
const float TILE = 0.75f;             // 155 * 0.20 ≈ 31 (si tu mapa es 155 de ancho)
const float WALL_HEIGHT = 4.0f;       // paredes más bajas
const float LIGHT_CUBE_SCALE = 0.50f;

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

    glm::vec3 move(0.0f);

    // mover en plano XZ (sin volar)
    glm::vec3 f = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 r = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));

    float v = camera.Speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += f * v;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= f * v;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= r * v;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += r * v;

    if (move.x != 0.0f || move.z != 0.0f)
        MoveWithCollision(move);


    // Linterna L
    static bool lPrevState = false;
    bool lState = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
    if (lState && !lPrevState) linternaEncendida = !linternaEncendida;
    lPrevState = lState;
}

// ================= MAPA DESDE ARCHIVO TXT =================
static std::vector<std::string> MAP;
static int MAP_W = 0;
static int MAP_H = 0;

static bool LoadMapFromTxt(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "No se pudo abrir el archivo: " << path << "\n";
        return false;
    }

    MAP.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && (line.back() == '\r')) line.pop_back(); // por si el txt tiene CRLF
        if (line.empty()) continue;
        MAP.push_back(line);
    }
    in.close();

    if (MAP.empty()) {
        std::cerr << "El archivo está vacio.\n";
        return false;
    }

    MAP_H = (int)MAP.size();
    MAP_W = (int)MAP[0].size();

    // Validación: todas las filas mismo ancho
    for (int r = 0; r < MAP_H; r++) {
        if ((int)MAP[r].size() != MAP_W) {
            std::cerr << "Error: fila " << r << " tiene ancho " << MAP[r].size()
                << " pero se esperaba " << MAP_W << "\n";
            return false;
        }
    }

    std::cout << "Mapa cargado OK: " << MAP_W << "x" << MAP_H << "\n";
    return true;
}

static inline bool InBounds(int r, int c) {
    return r >= 0 && c >= 0 && r < MAP_H && c < MAP_W;
}

static inline char Cell(int r, int c) {
    if (!InBounds(r, c)) return '0';
    return MAP[r][c];
}

static inline bool IsFloor(int r, int c) {
    char t = Cell(r, c);
    return t == '1' || t == 'S' || t == 's';
}



static inline bool IsWall(int r, int c) {
    char t = Cell(r, c);
    return t == '0' || t == '2' || t == '3'; // paredes por tipo
}

static inline int WallType(int r, int c) {
    char t = Cell(r, c);
    if (t == '0') return 0;
    if (t == '2') return 2;
    if (t == '3') return 3;
    return -1;
}


static glm::vec3 CellToWorld(int r, int c) {
    float halfW = (MAP_W * TILE) * 0.5f;
    float halfH = (MAP_H * TILE) * 0.5f;

    float x = (c * TILE) - halfW + TILE * 0.5f;   // +0.5 tile => centro
    float z = (halfH)-(r * TILE) - TILE * 0.5f; // -0.5 tile => centro
    return glm::vec3(x, 0.0f, z);
}



static inline bool WalkableCell(int r, int c) {
    return InBounds(r, c) && IsFloor(r, c); // SOLO suelo es caminable
}

// Convierte mundo (x,z) a celda (r,c) usando TU MISMO CellToWorld (sin invertir)


// Revisa colisión con 4 puntos del radio (circle approx)
// ==== COLISIONES (PLAYER CIRCLE vs WALL TILES) ====
const float PLAYER_RADIUS = 0.22f;  // sube/baja (con TILE=0.50, 0.12–0.18)
const float PLAYER_SKIN = 0.03f;  // margen para NO pegarse (evita que la cámara "asome")

static inline void TileAABB(int r, int c, float& xMin, float& xMax, float& zMin, float& zMax) {
    float halfW = (MAP_W * TILE) * 0.5f;
    float halfH = (MAP_H * TILE) * 0.5f;

    // x = c*TILE - halfW  (crece hacia +x)
    xMin = (c * TILE) - halfW;
    xMax = xMin + TILE;

    // z = halfH - r*TILE  (crece hacia +z cuando r baja)
    zMax = halfH - (r * TILE);
    zMin = zMax - TILE;
}

static inline bool CircleAABB(float cx, float cz, float radius, float xMin, float xMax, float zMin, float zMax) {
    // clamp
    float x = (cx < xMin) ? xMin : (cx > xMax ? xMax : cx);
    float z = (cz < zMin) ? zMin : (cz > zMax ? zMax : cz);

    float dx = cx - x;
    float dz = cz - z;
    return (dx * dx + dz * dz) <= (radius * radius);
}

static inline bool CollidesAt(float x, float z) {
    // revisa SOLO tiles cercanas (3x3) alrededor del jugador
    float halfW = (MAP_W * TILE) * 0.5f;
    float halfH = (MAP_H * TILE) * 0.5f;

    int c0 = (int)floor((x + halfW) / TILE);
    int r0 = (int)floor((halfH - z) / TILE);

    float rad = PLAYER_RADIUS + PLAYER_SKIN;

    for (int rr = r0 - 1; rr <= r0 + 1; rr++) {
        for (int cc = c0 - 1; cc <= c0 + 1; cc++) {
            // fuera del mapa = pared sólida
            if (!InBounds(rr, cc) || IsWall(rr, cc)) {
                float xMin, xMax, zMin, zMax;
                // si está fuera, igual calculamos AABB "virtual" en esa celda
                TileAABB(rr, cc, xMin, xMax, zMin, zMax);
                if (CircleAABB(x, z, rad, xMin, xMax, zMin, zMax))
                    return true;
            }
        }
    }
    return false;
}




// Movimiento con colisión y "slide": intenta X y Z por separado
static inline void MoveWithCollision(const glm::vec3& deltaXZ) {
    glm::vec3 pos = camera.Position;

    // pasos cortos para evitar tunneling
    float len = glm::length(glm::vec2(deltaXZ.x, deltaXZ.z));
    float maxStep = 0.02f; // mientras menor, más seguro
    int steps = (len > 0.0f) ? (int)ceil(len / maxStep) : 1;

    glm::vec3 step = deltaXZ / (float)steps;

    for (int i = 0; i < steps; i++) {
        // slide: X luego Z
        glm::vec3 tryX = pos + glm::vec3(step.x, 0.0f, 0.0f);
        if (!CollidesAt(tryX.x, tryX.z)) pos = tryX;

        glm::vec3 tryZ = pos + glm::vec3(0.0f, 0.0f, step.z);
        if (!CollidesAt(tryZ.x, tryZ.z)) pos = tryZ;
    }

    camera.Position = pos;
}



// Busca automáticamente un spawn: primera '1' encontrada (o si quieres, busca una marca 'S')
static bool FindSpawn(int& outR, int& outC) {
    // 1) si existe S o s, usarla
    for (int r = 0; r < MAP_H; r++) {
        for (int c = 0; c < MAP_W; c++) {
            if (MAP[r][c] == 'S' || MAP[r][c] == 's') {
                outR = r; outC = c;
                return true;
            }
        }
    }

    // 2) fallback: primera celda de suelo
    for (int r = 0; r < MAP_H; r++) {
        for (int c = 0; c < MAP_W; c++) {
            if (IsFloor(r, c)) { outR = r; outC = c; return true; }
        }
    }
    return false;
}



// ================= MAIN =================
int main() {
    // 1) Cargar mapa ANTES de crear OpenGL (así si falla, no pierdes tiempo)
    // Ruta recomendada: el archivo junto al .exe (o junto al proyecto ejecutando desde VS)
    if (!LoadMapFromTxt("maze.txt")) {
        std::cerr << "ERROR: No se pudo cargar maze.txt\n";
        return -1;
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LABERINTO (TXT) - Sin Texturas", nullptr, nullptr);
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
        // (igual que el tuyo)
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

    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &wallVAO);
    glGenBuffers(1, &wallVBO);
    glBindVertexArray(wallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    // ===== Spawn cámara (auto) =====
    int sr = 0, sc = 0;
    if (!FindSpawn(sr, sc)) { sr = 0; sc = 0; }
    glm::vec3 spawnW = CellToWorld(sr, sc);
    camera.SetPose(glm::vec3(spawnW.x, 2.0f, spawnW.z), 180.0f, 0.0f);

    // ================= LOOP =================
    while (!glfwWindowShouldClose(window)) {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;

        processInput(window);

        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.2f, 250.0f);
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
            shader.setVec3("spotLightPos", glm::vec3(0.0f, 0.0f, 0.0f));
            shader.setVec3("spotLightDir", glm::vec3(0.0f, 0.0f, 0.0f));

            shader.setFloat("spotCutOff", 0.0f);
            shader.setFloat("spotOuterCutOff", 0.0f);
        }

        // ===== DIBUJAR SUELO + PAREDES =====
        for (int r = 0; r < MAP_H; r++) {
            for (int c = 0; c < MAP_W; c++) {
                if (!IsFloor(r, c)) continue;


                glm::vec3 w = CellToWorld(r, c);

                // ----- Suelo -----
                glBindVertexArray(floorVAO);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(w.x, 0.0f, w.z));
                model = glm::scale(model, glm::vec3(TILE, 1.0f, TILE));
                shader.setMat4("model", model);

                shader.setVec3("baseColor", glm::vec3(0.78f, 0.65f, 0.20f));
                glDrawArrays(GL_TRIANGLES, 0, 6);

                // ----- Techo -----
                glBindVertexArray(floorVAO);
                glm::mat4 roof = glm::mat4(1.0f);
                roof = glm::translate(roof, glm::vec3(w.x, WALL_HEIGHT, w.z));
                roof = glm::scale(roof, glm::vec3(TILE, 1.0f, TILE));
                shader.setMat4("model", roof);

                // color del techo (gris)
                shader.setVec3("baseColor", 0.25f, 0.25f, 0.25f);
                glDrawArrays(GL_TRIANGLES, 0, 6);


                // ----- Paredes (bordes) -----
                glBindVertexArray(wallVAO);
                shader.setVec3("baseColor", glm::vec3(0.90f, 0.90f, 0.90f));

                // Norte (vecino r-1)
                if (!InBounds(r - 1, c) || IsWall(r - 1, c)) {
                    int t = InBounds(r - 1, c) ? WallType(r - 1, c) : 0; // fuera = tipo 0

                    glm::vec3 col = glm::vec3(0.10f); // tipo 0 (oscuro / “negro”)
                    if (t == 2) col = glm::vec3(0.10f, 0.65f, 0.95f);    // azul
                    if (t == 3) col = glm::vec3(0.95f, 0.20f, 0.20f);    // rojo
                    shader.setVec3("baseColor", col);

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x, 0.0f, w.z + TILE * 0.5f));
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
                // ===================== SUR =====================
                if (!InBounds(r + 1, c) || IsWall(r + 1, c)) {
                    int t = InBounds(r + 1, c) ? WallType(r + 1, c) : 0; // fuera = tipo 0

                    glm::vec3 col = glm::vec3(0.10f);                 // 0 = negro/oscuro
                    if (t == 2) col = glm::vec3(0.10f, 0.65f, 0.95f);  // 2 = azul
                    if (t == 3) col = glm::vec3(0.95f, 0.20f, 0.20f);  // 3 = rojo
                    shader.setVec3("baseColor", col);

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x, 0.0f, w.z - TILE * 0.5f));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }

                // ===================== OESTE =====================
                if (!InBounds(r, c - 1) || IsWall(r, c - 1)) {
                    int t = InBounds(r, c - 1) ? WallType(r, c - 1) : 0; // fuera = tipo 0

                    glm::vec3 col = glm::vec3(0.10f);
                    if (t == 2) col = glm::vec3(0.10f, 0.65f, 0.95f);
                    if (t == 3) col = glm::vec3(0.95f, 0.20f, 0.20f);
                    shader.setVec3("baseColor", col);

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x - TILE * 0.5f, 0.0f, w.z));
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0, 1, 0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }

                // ===================== ESTE =====================
                if (!InBounds(r, c + 1) || IsWall(r, c + 1)) {
                    int t = InBounds(r, c + 1) ? WallType(r, c + 1) : 0; // fuera = tipo 0

                    glm::vec3 col = glm::vec3(0.10f);
                    if (t == 2) col = glm::vec3(0.10f, 0.65f, 0.95f);
                    if (t == 3) col = glm::vec3(0.95f, 0.20f, 0.20f);
                    shader.setVec3("baseColor", col);

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
