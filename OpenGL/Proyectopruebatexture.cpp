#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/stb_image.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <queue>
#include <algorithm>

// Estructura simple para coordenadas de grid
struct Point { int r, c; };

// Estructura del Enemigo
struct Enemy {
    glm::vec3 pos;      // Posición suave (interpolada) en el mundo
    int r, c;           // Posición lógica actual en el grid
    float speed = 2.5f; // Velocidad de movimiento
};

// Variables globales para los enemigos
Enemy enemy1, enemy2;

// ================= CUBOS COLECCIONABLES =================
struct Collectible {
    glm::vec3 pos; // Posición en el mundo
    bool collected; // Si ya fue recogido
    int r, c; // Celda del grid
    bool isPreview = false; // Si es un preview (no cuenta en el total)
};

std::vector<Collectible> collectibles;
int collectedCount = 0;
const int TOTAL_COLLECTIBLES = 5;

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

// ================= MODO VISTA AEREA =================
bool modoAereo = false;
float alturaAerea = 50.0f;  // Altura cuando vuelas sobre el laberinto
float alturaOriginal = 2.0f; // Altura normal del jugador

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

    // Toggle modo aéreo con SPACE (subir)
    static bool spacePrevState = false;
    bool spaceState = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spaceState && !spacePrevState && !modoAereo) {
        modoAereo = true;
        camera.Position.y = alturaAerea;
        // Mirar hacia abajo
        camera.Pitch = -89.0f;
        camera.Mouse(0, 0); // Actualizar vectores de cámara
    }
    spacePrevState = spaceState;

    // Bajar con LEFT SHIFT
    static bool shiftPrevState = false;
    bool shiftState = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    if (shiftState && !shiftPrevState && modoAereo) {
        modoAereo = false;
        camera.Position.y = alturaOriginal;
        camera.Pitch = 0.0f;
        camera.Mouse(0, 0); // Actualizar vectores de cámara
    }
    shiftPrevState = shiftState;

    glm::vec3 move(0.0f);

    if (modoAereo) {
        // Movimiento libre en modo aéreo (sin colisiones)
        float v = camera.Speed * 3.0f * deltaTime; // Más rápido en el aire

        // Movimiento horizontal basado en la orientación
        glm::vec3 forward = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
        glm::vec3 right = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.Position += forward * v;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.Position -= forward * v;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.Position -= right * v;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.Position += right * v;

        // Subir/bajar con Q/E en modo aéreo
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.Position.y -= v;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.Position.y += v;
    }
    else {
        // Movimiento normal con colisiones
        glm::vec3 f = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
        glm::vec3 r = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));

        float v = camera.Speed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += f * v;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= f * v;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= r * v;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += r * v;

        if (move.x != 0.0f || move.z != 0.0f)
            MoveWithCollision(move);
    }

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

// helper: exit and spawn checks
static inline bool IsExit(int r, int c) { return Cell(r, c) == 'E'; }
static inline bool IsSpawn(int r, int c) { char t = Cell(r, c); return t == 'S' || t == 's'; }


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
// sube/baja (con TILE=0.50, 0.12–0.18)
const float PLAYER_RADIUS = 0.22f;
// margen para NO pegarse (evita que la cámara "asome")
const float PLAYER_SKIN = 0.03f;

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

// Convierte posición de Mundo a Grid (Inverso de CellToWorld)
static Point WorldToCell(glm::vec3 pos) {
    float halfW = (MAP_W * TILE) * 0.5f;
    float halfH = (MAP_H * TILE) * 0.5f;
    // Invertimos la fórmula de CellToWorld
    int c = (int)((pos.x + halfW) / TILE);
    int r = (int)((halfH - pos.z) / TILE);
    return { r, c };
}

// ALGORITMO BFS: Encuentra el siguiente paso inmediato hacia el objetivo
// Retorna la coordinada (r, c) a la que el enemigo debe moverse
static Point GetNextStepBFS(int startR, int startC, int targetR, int targetC) {
    // Si ya está en el destino, quedarse ahí
    if (startR == targetR && startC == targetC) return { startR, startC };

    // Direcciones: Arriba, Abajo, Izquierda, Derecha
    int dr[] = { -1, 1, 0, 0 };
    int dc[] = { 0, 0, -1, 1 };

    // Estructuras para BFS
    bool visited[200][200]; // Ajustar tamaño según tu mapa máximo o usar vector dinámico
    Point parent[200][200]; // Para reconstruir el camino

    // Inicializar visited en false (simple memset o loops)
    for (int i = 0; i < MAP_H; i++)
        for (int j = 0; j < MAP_W; j++) visited[i][j] = false;

    std::queue<Point> q;
    q.push({ startR, startC });
    visited[startR][startC] = true;
    parent[startR][startC] = { -1, -1 };

    bool found = false;

    while (!q.empty()) {
        Point curr = q.front();
        q.pop();

        if (curr.r == targetR && curr.c == targetC) {
            found = true;
            break;
        }

        // Explorar vecinos
        for (int i = 0; i < 4; i++) {
            int nr = curr.r + dr[i];
            int nc = curr.c + dc[i];

            // Validar límites y que sea suelo (IsFloor es tu función existente)
            if (InBounds(nr, nc) && !visited[nr][nc] && IsFloor(nr, nc)) {
                visited[nr][nc] = true;
                parent[nr][nc] = curr;
                q.push({ nr, nc });
            }
        }
    }

    if (!found) return { startR, startC }; // No hay camino

    // Reconstruir camino desde el Target hacia atrás hasta llegar al hijo del Start
    Point curr = { targetR, targetC };
    while (true) {
        Point p = parent[curr.r][curr.c];
        if (p.r == startR && p.c == startC) {
            return curr; // Este es el siguiente paso inmediato
        }
        curr = p;
    }
}

static void SpawnEnemies(glm::vec3 playerPos) {
    Point pCell = WorldToCell(playerPos);
    int spawnedCount = 0;

    // Distancias en celdas (Grid)
    int minDist = 5;  // No aparecer pegado al jugador
    int maxDist = 15; // No aparecer al otro lado del mapa

    // Intentos aleatorios para encontrar posición válida
    for (int i = 0; i < 1000; i++) {
        int r = rand() % MAP_H;
        int c = rand() % MAP_W;

        if (IsFloor(r, c)) {
            float dPlayer = glm::distance(glm::vec2(r, c), glm::vec2(pCell.r, pCell.c));

            if (dPlayer > minDist && dPlayer < maxDist) {
                // Configurar enemigo 1
                if (spawnedCount == 0) {
                    enemy1.r = r; enemy1.c = c;
                    enemy1.pos = CellToWorld(r, c);
                    spawnedCount++;
                }
                // Configurar enemigo 2 (verificar que no esté cerca del 1)
                else if (spawnedCount == 1) {
                    float dEnemy1 = glm::distance(glm::vec2(r, c), glm::vec2(enemy1.r, enemy1.c));
                    if (dEnemy1 > 5) { // Separados al menos 5 casillas
                        enemy2.r = r; enemy2.c = c;
                        enemy2.pos = CellToWorld(r, c);
                        spawnedCount++;
                        break;
                    }
                }
            }
        }
    }
    std::cout << "Enemigos spawneados: " << spawnedCount << "\n";
}

static void SpawnCollectibles(const glm::vec3& playerPos) {
    collectibles.clear();
    collectedCount = 0;

    Point pCell = WorldToCell(playerPos);

    int zonesR = TOTAL_COLLECTIBLES; // una zona por collectible
    int zoneHeight = std::max(1, MAP_H / zonesR);

    for (int zone = 0; zone < TOTAL_COLLECTIBLES; zone++) {
        int zoneStartR = zone * zoneHeight;
        int zoneEndR = (zone == zonesR - 1) ? MAP_H : std::min(MAP_H, (zone + 1) * zoneHeight);

        bool found = false;
        for (int attempts = 0; attempts < 500 && !found; attempts++) {
            int r = zoneStartR + (zoneEndR - zoneStartR > 0 ? rand() % (zoneEndR - zoneStartR) : 0);
            int c = rand() % std::max(1, MAP_W);

            if (InBounds(r, c) && IsFloor(r, c)) {
                float dPlayer = glm::distance(glm::vec2(r, c), glm::vec2(pCell.r, pCell.c));
                if (dPlayer > 3.0f) {
                    bool tooClose = false;
                    for (const auto& col : collectibles) {
                        float d = glm::distance(glm::vec2(r, c), glm::vec2(col.r, col.c));
                        if (d < 5.0f) { tooClose = true; break; }
                    }
                    if (!tooClose) {
                        Collectible newCol;
                        newCol.r = r; newCol.c = c;
                        newCol.pos = CellToWorld(r, c);
                        newCol.pos.y = 1.5f;
                        newCol.collected = false;
                        newCol.isPreview = false;
                        collectibles.push_back(newCol);
                        found = true;
                    }
                }
            }
        }
    }
    std::cout << "Cubos coleccionables spawneados: " << collectibles.size() << " de " << TOTAL_COLLECTIBLES << "\n";
}

static void CheckCollectibles(const glm::vec3& playerPos, GLFWwindow* window) {
    const float COLLECT_RADIUS = 0.8f;

    for (auto& col : collectibles) {
        if (col.collected) continue;

        float dist = glm::distance(glm::vec2(playerPos.x, playerPos.z), glm::vec2(col.pos.x, col.pos.z));
        if (dist < COLLECT_RADIUS) {
            col.collected = true;
            if (!col.isPreview) {
                collectedCount++;
            }

            // Actualizar título
            std::string title;
            if (collectedCount < TOTAL_COLLECTIBLES) {
                title = "LABERINTO - Cubos recogidos: " + std::to_string(collectedCount) + " de " + std::to_string(TOTAL_COLLECTIBLES);
                std::cout << "=== CUBO RECOGIDO! " << collectedCount << " de " << TOTAL_COLLECTIBLES << " cubos ===\n";
            }
            else {
                title = "LABERINTO - HAS RECOGIDO TODOS LOS CUBOS!!!";
                std::cout << "==============================================\n";
                std::cout << " HAS RECOGIDO TODOS LOS CUBOS!!! \n";
                std::cout << "==============================================\n";
            }
            if (window) glfwSetWindowTitle(window, title.c_str());
        }
    }
}


// ================= TEXTURAS =================
// simple texture loader (usable for start screen and map textures)
static unsigned int loadTexture(const char* path) {
    unsigned int id;
    glGenTextures(1, &id);

    int w, h, c;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &c, 0);
    if (!data) {
        // Retry without ../
        std::string sPath = path;
        if (sPath.size() > 3 && sPath.substr(0, 3) == "../") {
            std::string fallback = sPath.substr(3);
            data = stbi_load(fallback.c_str(), &w, &h, &c, 0);
        }
    }

    if (data) {
        GLenum format = GL_RGB;
        if (c == 1) format = GL_RED;
        else if (c == 3) format = GL_RGB;
        else if (c == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
    }
    stbi_image_free(data);
    return id;
}

// ================= MAIN =================

int main() {
    if (!LoadMapFromTxt("maze.txt")) { std::cerr << "ERROR: No se pudo cargar maze.txt\n"; return -1; }


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
    if (shader.ID == 0) shader = Shader("shaders/B2T3.vs", "shaders/B2T3.fs");

    Shader lightShader("shaders/light_cube.vs", "shaders/light_cube.fs");
    if (lightShader.ID == 0) lightShader = Shader("shaders/light_cube.vs", "shaders/light_cube.fs");

    // --- Pantalla de inicio ---
    Shader screenShader("shaders/screen.vs", "shaders/screen.fs");
    if (screenShader.ID == 0) screenShader = Shader("shaders/screen.vs", "shaders/screen.fs");
    float screenQuad[] = {
        // positions // texcoords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f
    };

    unsigned int screenVAO, screenVBO;
    glGenVertexArrays(1, &screenVAO);
    glGenBuffers(1, &screenVBO);
    glBindVertexArray(screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuad), screenQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    unsigned int startTex = loadTexture("textures/portada.png");
    screenShader.use();
    screenShader.setInt("screenTex", 0);

    // ===== TEXTURAS =====
    unsigned int floorRoomTex = loadTexture("textures/TextruaP1.png");
    // restore: room walls use Pared4, hall walls use multiple pasillo textures
    unsigned int wallRoomTex = loadTexture("textures/Pared4.png");
    unsigned int floorHallTex = loadTexture("textures/gradas1.png");

    // load a sequence of pasillo textures (pattern: pasillo1..pasillo6)
    unsigned int wallHallTex[6];
    wallHallTex[0] = loadTexture("textures/pasillo1.png");
    wallHallTex[1] = loadTexture("textures/pasillo2.png");
    wallHallTex[2] = loadTexture("textures/pasillo3.png");
    wallHallTex[3] = loadTexture("textures/pasillo4.png");
    wallHallTex[4] = loadTexture("textures/pasillo5.png");
    wallHallTex[5] = loadTexture("textures/pasillo6.png");

    // make the texture that's now used for hall walls clamp so it doesn't repeat when UV outside0..1
    // NOTE: keep clamp for wallRoomTex as before
    glBindTexture(GL_TEXTURE_2D, wallRoomTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    unsigned int wallEndTex = loadTexture("textures/Finalpa2.png"); // pared final
    unsigned int harrybotTex = loadTexture("textures/Francisbot.png");
    // startTex ya cargada arriba para la pantalla

    // Asegurar que el shader use la unidad de textura0 para 'texture1'
    shader.use();
    shader.setInt("texture1", 0);

    bool gameStarted = false;
    bool lastEnter = false;

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


    // === NUEVO: INICIALIZAR ENEMIGOS ===
    srand((unsigned int)glfwGetTime()); // Semilla random
    SpawnEnemies(camera.Position);

    // Spawnear coleccionables y crear un preview extra junto al jugador
    SpawnCollectibles(camera.Position);
    // crear un collectible preview al lado del jugador para ver cómo es
    Collectible preview;
    // colocar1 unidad delante de la cámara en XZ
    glm::vec3 ahead = camera.Position + glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z)) * (TILE * 0.75f);
    preview.pos = ahead;
    preview.pos.y = 1.5f;
    preview.collected = false;
    preview.isPreview = true;
    Point pc = WorldToCell(preview.pos);
    preview.r = pc.r; preview.c = pc.c;
    collectibles.push_back(preview);

    // ================= LOOP =================
    while (!glfwWindowShouldClose(window)) {
        // Poll events first so key state is updated for start screen
        glfwPollEvents();

        // check enter to start
        bool enter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (enter && !lastEnter) gameStarted = true;
        lastEnter = enter;

        if (!gameStarted) {
            // render only the start screen
            glClear(GL_COLOR_BUFFER_BIT);
            screenShader.use();
            glBindVertexArray(screenVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, startTex);

            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);

            glfwSwapBuffers(window);
            continue;
        }

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

                shader.setBool("useTexture", true);
                shader.setBool("useWorldUV", false);
                shader.setVec2("texScale2", glm::vec2(0.5f, 0.5f));
                shader.setBool("flipTexY", false);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, floorRoomTex);
                shader.setVec3("baseColor", glm::vec3(1.0f));
                glDrawArrays(GL_TRIANGLES, 0, 6);

                shader.setBool("useTexture", false);
                shader.setVec2("texScale2", glm::vec2(1.0f, 1.0f));

                // ----- Techo (sin textura) -----
                glBindVertexArray(floorVAO);
                glm::mat4 roof = glm::mat4(1.0f);
                roof = glm::translate(roof, glm::vec3(w.x, WALL_HEIGHT, w.z));
                roof = glm::scale(roof, glm::vec3(TILE, 1.0f, TILE));
                shader.setMat4("model", roof);
                shader.setBool("useTexture", false);
                shader.setVec3("baseColor", 0.25f, 0.25f, 0.25f);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                // ----- Paredes (bordes) por cara -----
                glBindVertexArray(wallVAO);
                shader.setVec3("baseColor", glm::vec3(0.90f, 0.90f, 0.90f));

                // Norte (r-1)
                if (!InBounds(r -1, c) || IsWall(r -1, c)) {
                    int nr = r -1;
                    int nc = c;
                    int t = InBounds(nr, nc) ? WallType(nr, nc) :0;
                    unsigned int wallTex;
                    if (t ==2) wallTex = wallRoomTex;
                    else {
                        int idx = ((nc %6) +6) %6; // pattern across columns
                        wallTex = wallHallTex[idx];
                    }

                    shader.setBool("useTexture", true);
                    shader.setBool("useWorldUV", false);
                    shader.setVec2("texScale2", glm::vec2(1.0f,1.0f));
                    // flip vertically only for hall textures to fix inverted pattern
                    shader.setBool("flipTexY", (t ==2) ? false : true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTex);
                    shader.setVec3("baseColor", glm::vec3(1.0f));

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x,0.0f, w.z + TILE *0.5f));
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0,1,0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT,1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES,0,6);

                    shader.setBool("useTexture", false);
                }

                // Sur (r+1)
                if (!InBounds(r +1, c) || IsWall(r +1, c)) {
                    int nr = r +1;
                    int nc = c;
                    int t = InBounds(nr, nc) ? WallType(nr, nc) :0;
                    unsigned int wallTex;
                    if (t ==2) wallTex = wallRoomTex;
                    else {
                        int idx = ((nc %6) +6) %6;
                        wallTex = wallHallTex[idx];
                    }

                    shader.setBool("useTexture", true);
                    shader.setBool("useWorldUV", false);
                    shader.setVec2("texScale2", glm::vec2(1.0f,1.0f));
                    shader.setBool("flipTexY", (t ==2) ? false : true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTex);
                    shader.setVec3("baseColor", glm::vec3(1.0f));

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x,0.0f, w.z - TILE *0.5f));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT,1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES,0,6);

                    shader.setBool("useTexture", false);
                }

                // Oeste (c-1)
                if (!InBounds(r, c -1) || IsWall(r, c -1)) {
                    int nr = r;
                    int nc = c -1;
                    int t = InBounds(nr, nc) ? WallType(nr, nc) :0;
                    unsigned int wallTex;
                    if (t ==2) wallTex = wallRoomTex;
                    else {
                        int idx = ((nc %6) +6) %6;
                        wallTex = wallHallTex[idx];
                    }

                    shader.setBool("useTexture", true);
                    shader.setBool("useWorldUV", false);
                    shader.setVec2("texScale2", glm::vec2(1.0f,1.0f));
                    shader.setBool("flipTexY", (t ==2) ? false : true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTex);
                    shader.setVec3("baseColor", glm::vec3(1.0f));

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x - TILE *0.5f,0.0f, w.z));
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0,1,0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT,1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES,0,6);

                    shader.setBool("useTexture", false);
                }

                // Este (c+1)
                if (!InBounds(r, c +1) || IsWall(r, c +1)) {
                    int nr = r;
                    int nc = c +1;
                    int t = InBounds(nr, nc) ? WallType(nr, nc) :0;
                    unsigned int wallTex;
                    if (t ==2) wallTex = wallRoomTex;
                    else {
                        int idx = ((nc %6) +6) %6;
                        wallTex = wallHallTex[idx];
                    }

                    shader.setBool("useTexture", true);
                    shader.setBool("useWorldUV", false);
                    shader.setVec2("texScale2", glm::vec2(1.0f,1.0f));
                    shader.setBool("flipTexY", (t ==2) ? false : true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTex);
                    shader.setVec3("baseColor", glm::vec3(1.0f));

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x + TILE *0.5f,0.0f, w.z));
                    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT,1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES,0,6);

                    shader.setBool("useTexture", false);
                }

            }
        }

        // ================= ACTUALIZAR ENEMIGOS (IA) =================
        // Haremos que recalcule el camino frame a frame hacia el centro de la siguiente celda
        Point playerGrid = WorldToCell(camera.Position);

        Enemy* enemies[] = { &enemy1, &enemy2 };
        for (Enemy* e : enemies) {
            // 1. Calcular siguiente casilla ideal con BFS
            Point nextCell = GetNextStepBFS(e->r, e->c, playerGrid.r, playerGrid.c);

            // 2. Obtener posición world del centro de esa casilla
            glm::vec3 targetWorld = CellToWorld(nextCell.r, nextCell.c);

            // 3. Moverse suavemente hacia ese objetivo
            glm::vec3 dir = targetWorld - e->pos;
            if (glm::length(dir) > 0.01f) {
                dir = glm::normalize(dir);
                e->pos += dir * e->speed * deltaTime;
            }

            // 4. Si está muy cerca del centro de la casilla objetivo, actualizar su grid lógico
            if (glm::distance(e->pos, targetWorld) < 0.1f) {
                e->r = nextCell.r;
                e->c = nextCell.c;
            }
        }

        // ================= DIBUJAR ENEMIGOS =================
        // Usamos el shader principal (tiene luz y sombras)
        shader.use();

        // Color rojo amenazante para los enemigos
        shader.setVec3("baseColor", glm::vec3(0.8f, 0.0f, 0.0f));

        for (Enemy* e : enemies) {
            glBindVertexArray(cubeVAO); // Reusamos el cubo de la luz
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, e->pos);
            // Hacemos el enemigo un poco más pequeño que el Tile para que no atraviese paredes visualmente
            model = glm::scale(model, glm::vec3(0.4f));
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
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

        // ================= RECOLECCIÓN Y DIBUJO DE CUBOS COLECCIONABLES =================
        CheckCollectibles(camera.Position, window);

        // Dibujar solo si hay cubos no recogidos
        if (collectibles.size() > 0) {
            glBindVertexArray(cubeVAO);
            shader.use();
            shader.setVec3("baseColor", glm::vec3(0.8f, 0.8f, 0.0f)); // Color amarillo para los cubos

            for (const auto& col : collectibles) {
                if (col.collected) continue; // Solo dibujar los no recogidos

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, col.pos);
                model = glm::scale(model, glm::vec3(0.2f)); // Tamaño más pequeño para los cubos
                shader.setMat4("model", model);

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
