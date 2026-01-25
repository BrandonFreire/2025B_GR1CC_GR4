// ===================== LABERINTO DESDE TXT - SIN TEXTURAS =====================
// Lee un archivo maze.txt (0 = vacío, 1 = suelo, E = salida)
// Requiere B2T3.fs con uniform vec3 baseColor (sin samplear texture1).

// ===================== FORZAR GPU DEDICADA =====================
// Estas exportaciones indican a los drivers de NVIDIA y AMD que
// utilicen la tarjeta gráfica dedicada en lugar de la integrada.
#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
}
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/model_animation.h>   
#include <learnopengl/animation.h>          
#include <learnopengl/animator.h>  

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <queue>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/stb_image.h>

// Estructura simple para coordenadas de grid
struct Point { int r, c; };

// Estructura del Enemigo
struct Enemy {
    glm::vec3 pos;      // Posición suave (interpolada) en el mundo
    int r, c;           // Posición lógica actual en el grid
    float speed = 2.5f; // Velocidad de movimiento

    // Variables para animación
    float animTime = 0.0f;     // Tiempo actual de la animación
    float rotation = 0.0f;     // Rotación hacia donde mira el enemigo  <-- ESTE CAMPO
    glm::vec3 lastPos;         // Posición anterior
    bool isMoving = false;     // Si está en movimiento

    // ===== OPTIMIZACIÓN BFS: Caché del camino =====
    std::vector<Point> cachedPath;  // Camino cacheado hacia el jugador
    int lastPlayerR = -1;           // Última posición conocida del jugador (fila)
    int lastPlayerC = -1;           // Última posición conocida del jugador (columna)
    int lastEnemyR = -1;            // Última posición del enemigo cuando se calculó el camino
    int lastEnemyC = -1;            // Última posición del enemigo cuando se calculó el camino
    float pathRecalcTimer = 0.0f;   // Timer para recalcular camino periódicamente
};

// Variables globales para los enemigos
Enemy enemy1;//, enemy2;

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

// ================= MINIMAPA =================
const int MINIMAP_RADIUS = 8;
const float MINIMAP_SIZE = 200.0f;
const float MINIMAP_MARGIN = 20.0f;
const float MINIMAP_CELL_SIZE = MINIMAP_SIZE / (MINIMAP_RADIUS * 2 + 1);

// ================= MOVIMIENTO ALEATORIO ENEMY1 =================
Point enemy1RandomTarget = {0, 0};
bool enemy1HasTarget = false;

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

// =====================================================================
// ALGORITMO A* OPTIMIZADO CON PERSECUCIÓN INTELIGENTE
// =====================================================================
// Ventajas sobre BFS:
// 1. A* usa heurística (distancia Manhattan) para explorar menos nodos
// 2. Persecución directa cuando hay línea de visión (evita pathfinding)
// 3. Caché inteligente que solo recalcula cuando es necesario
// 4. Early-exit cuando el enemigo está cerca del jugador
// =====================================================================

#include <unordered_set>
#include <cmath>

// Estructura para nodos del A*
struct AStarNode {
    int r, c;
    int g;  // Costo desde el inicio
    int f;  // g + h (costo total estimado)
    
    // Para priority_queue (menor f tiene mayor prioridad)
    bool operator>(const AStarNode& other) const {
        return f > other.f;
    }
};

// Constantes de pathfinding optimizadas
const int PATHFIND_MAX_NODES = 800;        // Máximo nodos a explorar (reduce CPU)
const float PATH_RECALC_INTERVAL = 0.3f;   // Recalcular cada 0.3s (más que antes)
const int DIRECT_CHASE_DISTANCE = 3;       // Distancia para persecución directa
const int LINE_OF_SIGHT_CHECK = 8;         // Máximo para verificar línea de visión

// Buffers estáticos reutilizables para A*
static std::vector<std::vector<int>> astarG;      // Costo g por celda
static std::vector<std::vector<bool>> astarClosed; // Nodos cerrados
static std::vector<std::vector<Point>> astarParent; // Padres para reconstruir
static bool astarBuffersInit = false;

// Inicializar buffers A* una sola vez
static void InitAStarBuffers() {
    if (astarBuffersInit && (int)astarG.size() == MAP_H) return;
    
    astarG.assign(MAP_H, std::vector<int>(MAP_W, INT_MAX));
    astarClosed.assign(MAP_H, std::vector<bool>(MAP_W, false));
    astarParent.assign(MAP_H, std::vector<Point>(MAP_W, {-1, -1}));
    astarBuffersInit = true;
}

// Limpiar solo región necesaria (optimización clave)
static void ClearAStarRegion(int r1, int c1, int r2, int c2, int margin = 5) {
    int minR = std::max(0, std::min(r1, r2) - margin);
    int maxR = std::min(MAP_H - 1, std::max(r1, r2) + margin);
    int minC = std::max(0, std::min(c1, c2) - margin);
    int maxC = std::min(MAP_W - 1, std::max(c1, c2) + margin);
    
    for (int r = minR; r <= maxR; r++) {
        for (int c = minC; c <= maxC; c++) {
            astarG[r][c] = INT_MAX;
            astarClosed[r][c] = false;
            astarParent[r][c] = {-1, -1};
        }
    }
}

// Heurística: Distancia Manhattan (admisible para grid 4-direccional)
static inline int Heuristic(int r1, int c1, int r2, int c2) {
    return abs(r1 - r2) + abs(c1 - c2);
}

// Verificar línea de visión entre dos puntos (Bresenham simplificado)
// Si hay línea de visión directa, no necesitamos pathfinding
static bool HasLineOfSight(int r1, int c1, int r2, int c2) {
    int dr = abs(r2 - r1);
    int dc = abs(c2 - c1);
    
    // Limitar distancia de verificación
    if (dr + dc > LINE_OF_SIGHT_CHECK) return false;
    
    int sr = (r1 < r2) ? 1 : -1;
    int sc = (c1 < c2) ? 1 : -1;
    
    int err = dr - dc;
    int r = r1, c = c1;
    
    while (r != r2 || c != c2) {
        // Verificar si la celda actual es caminable
        if (!InBounds(r, c) || !IsFloor(r, c)) return false;
        
        int e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r += sr; }
        if (e2 < dr)  { err += dr; c += sc; }
    }
    
    return InBounds(r2, c2) && IsFloor(r2, c2);
}

// ALGORITMO A* OPTIMIZADO
static std::vector<Point> ComputePathAStar(int startR, int startC, int targetR, int targetC) {
    std::vector<Point> path;
    
    // Caso trivial: ya estamos en el destino
    if (startR == targetR && startC == targetC) return path;
    
    // Early exit: si están muy lejos, no buscar (el enemigo se moverá hacia el jugador de forma aproximada)
    int manhattan = Heuristic(startR, startC, targetR, targetC);
    if (manhattan > 60) {
        // Mover hacia la dirección general del jugador
        int bestR = startR, bestC = startC;
        int bestDist = manhattan;
        
        const int dr[] = {-1, 1, 0, 0};
        const int dc[] = {0, 0, -1, 1};
        
        for (int i = 0; i < 4; i++) {
            int nr = startR + dr[i];
            int nc = startC + dc[i];
            if (InBounds(nr, nc) && IsFloor(nr, nc)) {
                int dist = Heuristic(nr, nc, targetR, targetC);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestR = nr;
                    bestC = nc;
                }
            }
        }
        
        if (bestR != startR || bestC != startC) {
            path.push_back({bestR, bestC});
        }
        return path;
    }
    
    // Verificar línea de visión directa (evita A* completamente)
    if (HasLineOfSight(startR, startC, targetR, targetC)) {
        // Construir camino directo
        int r = startR, c = startC;
        while (r != targetR || c != targetC) {
            int dr = (targetR > r) ? 1 : (targetR < r) ? -1 : 0;
            int dc = (targetC > c) ? 1 : (targetC < c) ? -1 : 0;
            
            // Preferir movimiento en una dirección a la vez
            if (dr != 0 && InBounds(r + dr, c) && IsFloor(r + dr, c)) {
                r += dr;
            } else if (dc != 0 && InBounds(r, c + dc) && IsFloor(r, c + dc)) {
                c += dc;
            } else {
                break; // No hay camino directo
            }
            path.push_back({r, c});
        }
        if (!path.empty()) return path;
    }
    
    // A* completo cuando no hay línea de visión
    InitAStarBuffers();
    ClearAStarRegion(startR, startC, targetR, targetC, 10);
    
    // Priority queue (min-heap por f)
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;
    
    // Direcciones: Arriba, Abajo, Izquierda, Derecha
    const int dr[] = {-1, 1, 0, 0};
    const int dc[] = {0, 0, -1, 1};
    
    // Iniciar desde el punto de inicio
    astarG[startR][startC] = 0;
    int h = Heuristic(startR, startC, targetR, targetC);
    openSet.push({startR, startC, 0, h});
    
    bool found = false;
    int nodesExplored = 0;
    
    while (!openSet.empty() && nodesExplored < PATHFIND_MAX_NODES) {
        AStarNode current = openSet.top();
        openSet.pop();
        
        // Si ya procesamos este nodo, saltar
        if (astarClosed[current.r][current.c]) continue;
        astarClosed[current.r][current.c] = true;
        nodesExplored++;
        
        // ¿Llegamos al destino?
        if (current.r == targetR && current.c == targetC) {
            found = true;
            break;
        }
        
        // Explorar vecinos
        for (int i = 0; i < 4; i++) {
            int nr = current.r + dr[i];
            int nc = current.c + dc[i];
            
            if (!InBounds(nr, nc) || !IsFloor(nr, nc) || astarClosed[nr][nc]) continue;
            
            int tentativeG = current.g + 1;
            
            if (tentativeG < astarG[nr][nc]) {
                astarG[nr][nc] = tentativeG;
                astarParent[nr][nc] = {current.r, current.c};
                int f = tentativeG + Heuristic(nr, nc, targetR, targetC);
                openSet.push({nr, nc, tentativeG, f});
            }
        }
    }
    
    if (!found) {
        // No encontramos camino completo, pero podemos movernos hacia el mejor nodo explorado
        // Buscar el nodo cerrado más cercano al objetivo
        int bestR = startR, bestC = startC;
        int bestH = Heuristic(startR, startC, targetR, targetC);
        
        // Buscar en región pequeña alrededor del inicio
        for (int dr = -5; dr <= 5; dr++) {
            for (int dc = -5; dc <= 5; dc++) {
                int r = startR + dr;
                int c = startC + dc;
                if (InBounds(r, c) && astarClosed[r][c]) {
                    int h = Heuristic(r, c, targetR, targetC);
                    if (h < bestH) {
                        bestH = h;
                        bestR = r;
                        bestC = c;
                    }
                }
            }
        }
        
        // Reconstruir camino hacia el mejor nodo encontrado
        if (bestR != startR || bestC != startC) {
            Point curr = {bestR, bestC};
            while (curr.r != startR || curr.c != startC) {
                path.push_back(curr);
                curr = astarParent[curr.r][curr.c];
                if (curr.r == -1) break;
            }
            std::reverse(path.begin(), path.end());
        }
        return path;
    }
    
    // Reconstruir camino desde el objetivo
    Point curr = {targetR, targetC};
    while (curr.r != startR || curr.c != startC) {
        path.push_back(curr);
        curr = astarParent[curr.r][curr.c];
        if (curr.r == -1) break;
    }
    std::reverse(path.begin(), path.end());
    
    return path;
}

// Función principal de pathfinding con caché inteligente
static Point GetNextStepAStar(Enemy& e, int targetR, int targetC, float dt) {
    e.pathRecalcTimer += dt;
    
    // Calcular distancia al jugador
    int distToPlayer = Heuristic(e.r, e.c, targetR, targetC);
    
    // OPTIMIZACIÓN 1: Persecución directa si está muy cerca
    if (distToPlayer <= DIRECT_CHASE_DISTANCE) {
        // Buscar la celda adyacente que más nos acerque
        const int dr[] = {-1, 1, 0, 0};
        const int dc[] = {0, 0, -1, 1};
        
        int bestR = e.r, bestC = e.c;
        int bestDist = distToPlayer;
        
        for (int i = 0; i < 4; i++) {
            int nr = e.r + dr[i];
            int nc = e.c + dc[i];
            if (InBounds(nr, nc) && IsFloor(nr, nc)) {
                int dist = Heuristic(nr, nc, targetR, targetC);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestR = nr;
                    bestC = nc;
                }
            }
        }
        
        return {bestR, bestC};
    }
    
    // OPTIMIZACIÓN 2: Verificar si el caché sigue siendo válido
    bool needRecalc = false;
    
    // Recalcular si el jugador se movió significativamente (más de 2 celdas)
    int playerMoved = Heuristic(e.lastPlayerR, e.lastPlayerC, targetR, targetC);
    if (playerMoved > 2) {
        needRecalc = true;
    }
    
    // Recalcular si el caché está vacío
    if (e.cachedPath.empty()) {
        needRecalc = true;
    }
    
    // Recalcular periódicamente (pero con intervalo más largo)
    if (e.pathRecalcTimer >= PATH_RECALC_INTERVAL) {
        needRecalc = true;
        e.pathRecalcTimer = 0.0f;
    }
    
    // OPTIMIZACIÓN 3: Actualizar caché cuando el enemigo se mueve
    if (e.lastEnemyR != e.r || e.lastEnemyC != e.c) {
        // Remover pasos ya completados del caché
        while (!e.cachedPath.empty()) {
            Point& front = e.cachedPath.front();
            if (front.r == e.r && front.c == e.c) {
                e.cachedPath.erase(e.cachedPath.begin());
            } else {
                break;
            }
        }
        e.lastEnemyR = e.r;
        e.lastEnemyC = e.c;
    }
    
    // Recalcular si es necesario
    if (needRecalc) {
        e.cachedPath = ComputePathAStar(e.r, e.c, targetR, targetC);
        e.lastPlayerR = targetR;
        e.lastPlayerC = targetC;
        e.lastEnemyR = e.r;
        e.lastEnemyC = e.c;
        e.pathRecalcTimer = 0.0f;
    }
    
    // Retornar siguiente paso
    if (!e.cachedPath.empty()) {
        return e.cachedPath.front();
    }
    
    // Sin camino, quedarse en lugar
    return {e.r, e.c};
}

// Mantener compatibilidad con función anterior
static Point GetNextStepBFS(int startR, int startC, int targetR, int targetC) {
    // Redirigir a A*
    Enemy tempEnemy;
    tempEnemy.r = startR;
    tempEnemy.c = startC;
    tempEnemy.lastPlayerR = -1;
    tempEnemy.lastPlayerC = -1;
    tempEnemy.lastEnemyR = startR;
    tempEnemy.lastEnemyC = startC;
    tempEnemy.pathRecalcTimer = 999.0f; // Forzar recálculo
    
    return GetNextStepAStar(tempEnemy, targetR, targetC, 0.0f);
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
                /*else if (spawnedCount == 1) {
                    float dEnemy1 = glm::distance(glm::vec2(r, c), glm::vec2(enemy1.r, enemy1.c));
                    if (dEnemy1 > 5) { // Separados al menos 5 casillas
                        enemy2.r = r; enemy2.c = c;
                        enemy2.pos = CellToWorld(r, c);
                        spawnedCount++;
                        break;
                    }
                }*/
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
static unsigned int loadTexture(const char* path, bool flip = true) {
    unsigned int id;
    glGenTextures(1, &id);

    int w, h, c;
    stbi_set_flip_vertically_on_load(flip);
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
    if (shader.ID == 0) shader = Shader("shaders/B2T3.vs", "shaders/B2T3.fs");

    Shader lightShader("shaders/light_cube.vs", "shaders/light_cube.fs");
    if (lightShader.ID == 0) lightShader = Shader("shaders/light_cube.vs", "shaders/light_cube.fs");

    // --- Pantalla de inicio ---
    Shader screenShader("shaders/screen.vs", "shaders/screen.fs");
    if (screenShader.ID == 0) screenShader = Shader("shaders/screen.vs", "shaders/screen.fs");

    // --- Shader del minimapa ---
    Shader minimapShader("shaders/minimap.vs", "shaders/minimap.fs");

    // VAO para el minimapa
    float minimapQuad[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f
    };
    unsigned int minimapVAO, minimapVBO;
    glGenVertexArrays(1, &minimapVAO);
    glGenBuffers(1, &minimapVBO);
    glBindVertexArray(minimapVAO);
    glBindBuffer(GL_ARRAY_BUFFER, minimapVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(minimapQuad), minimapQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

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

    unsigned int wallindie1 = loadTexture("textures/Finalpainsano1.png");
    unsigned int wallindie2 = loadTexture("textures/Finalpainsano2.png");

    // ===== TEXTURAS =====
    unsigned int floorRoomTex = loadTexture("textures/TextruaP1.png");
    // restore: room walls use Pared4, hall walls use multiple pasillo textures
    // load wallRoomTex without vertical flip so it displays upright
    unsigned int wallRoomTex = loadTexture("textures/Pared4.png", false);
    unsigned int floorHallTex = loadTexture("textures/gradas1.png");

    // load a sequence of pasillo textures (pattern: pasillo1..pasillo6)
    unsigned int wallHallTex[6];
    wallHallTex[0] = loadTexture("textures/polipa.png");
    wallHallTex[1] = loadTexture("textures/polipa2.png");
    wallHallTex[2] = loadTexture("textures/polipa3.png");
    wallHallTex[3] = loadTexture("textures/polipa4.png");
    wallHallTex[4] = loadTexture("textures/polipa5.png");
    wallHallTex[5] = loadTexture("textures/polipa6.png");

    // detecta algunos segmentos de pared estrechos y márcalos para usar la textura indie
    struct IndieSeg { int startR; int startC; int dir; int len; }; // dir:0=horizontal(segmento a lo largo de las columnas, atado a la fila del suelo),1=vertical(segmento a lo largo de las filas, atado a la columna del suelo)
    std::vector<IndieSeg> indieSegs;
    const int MAX_INDIE = 4;
    const int NARROW_THRESHOLD = 2; // longitud de segmento <= umbral considerado "estrecho"

    for (int rr = 0; rr < MAP_H && (int)indieSegs.size() < MAX_INDIE; rr++) {
        for (int cc = 0; cc < MAP_W && (int)indieSegs.size() < MAX_INDIE; cc++) {
            if (!IsFloor(rr, cc)) continue;
            // pared horizontal sobre esta celda de suelo (facing north)
            if (!InBounds(rr - 1, cc) || IsWall(rr - 1, cc)) {
                int startC = cc;
                while (startC - 1 >= 0 && IsFloor(rr, startC - 1) && (!InBounds(rr - 1, startC - 1) || IsWall(rr - 1, startC - 1))) startC--;
                int len = 0;
                int c2 = startC;
                while (c2 < MAP_W && IsFloor(rr, c2) && (!InBounds(rr - 1, c2) || IsWall(rr - 1, c2))) { len++; c2++; }
                // only mark segments that are exactly NARROW_THRESHOLD long (pairs)
                if (len == NARROW_THRESHOLD) indieSegs.push_back({ rr, startC,0, len });
            }
            if ((int)indieSegs.size() >= MAX_INDIE) break;

            // pared vertical a la izquierda de esta celda de suelo (facing west)
            if (!InBounds(rr, cc - 1) || IsWall(rr, cc - 1)) {
                int startR = rr;
                while (startR - 1 >= 0 && IsFloor(startR - 1, cc) && (!InBounds(startR - 1, cc - 1) || IsWall(startR - 1, cc - 1))) startR--;
                int len = 0;
                int r2 = startR;
                while (r2 < MAP_H && IsFloor(r2, cc) && (!InBounds(r2, cc - 1) || IsWall(r2, cc - 1))) { len++; r2++; }
                // only mark segments that are exactly NARROW_THRESHOLD long (pairs)
                if (len == NARROW_THRESHOLD) indieSegs.push_back({ startR, cc,1, len });
            }
        }
    }

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

    // USAR UN SHADER ESTÁNDAR PARA MODELOS 3D
    // (Asegúrate de tener "model_loading.vs" y "model_loading.fs" en tu carpeta shaders,
    //  son los shaders por defecto de LearnOpenGL para modelos).
    Shader xenomorphShader("shaders/model_loading.vs", "shaders/model_loading.fs");

    // CARGAR EL MODELO DEL ALIEN
    // La ruta debe coincidir con tu carpeta: model -> alien -> scene.gltf
    Model xenomorphModel("model/xenomorph/xenomorph.gltf");

    // CARGAR LA ANIMACIÓN (usa el mismo archivo GLTF)
    Animation xenomorphAnimation("model/xenomorph/xenomorph.gltf", &xenomorphModel);

    // CREAR EL ANIMATOR
    Animator animator(&xenomorphAnimation);

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

        // ===== PANTALLA DE INICIO (PORTADA) =====
        if (!gameStarted) {
            bool enter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
            if (enter && !lastEnter) {
                gameStarted = true;
                lastFrame = (float)glfwGetTime();
            }
            lastEnter = enter;

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
        shader.setVec3("lightPos", glm::vec3(0.0f, -1000.0f, 0.0f)); // Luz global desactivada
        shader.setVec3("viewPos", camera.Position);

        // compute wide stripe width for hall textures (much thicker stripes)
        int hallStripeWidth = std::max(1, MAP_W / 6); // base slice
        const int HALL_STRIPE_THICKNESS = 3; // make stripes3x wider
        hallStripeWidth = std::min(MAP_W, hallStripeWidth * HALL_STRIPE_THICKNESS);

        // Linterna simple sin parpadeo
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
                if (!InBounds(r - 1, c) || IsWall(r - 1, c)) {
                    int nr = r - 1;
                    int nc = c;
                    int t = InBounds(nr, nc) ? WallType(nr, nc) : 0;
                    unsigned int wallTex;
                    if (t == 2) wallTex = wallRoomTex;
                    else {
                        // check if this floor cell belongs to a marked indie horizontal segment
                        bool isIndie = false;
                        for (const auto& s : indieSegs) {
                            if (s.dir == 0 && s.startR == r && nc >= s.startC && nc < s.startC + s.len) { isIndie = true; break; }
                        }
                        if (isIndie) {
                            // find which tile of the pair this is and select appropriate indie texture
                            int chosen = 0; //0 -> first (start),1 -> second
                            for (const auto& s : indieSegs) {
                                if (s.dir == 0 && s.startR == r && nc >= s.startC && nc < s.startC + s.len) { chosen = nc - s.startC; break; }
                            }
                            wallTex = (chosen == 0) ? wallindie1 : wallindie2;
                        }
                        else {
                            // find start of continuous horizontal wall segment (scan left)
                            int startC = nc;
                            while (startC - 1 >= 0 && IsFloor(r, startC - 1) && IsWall(r - 1, startC - 1)) startC--;
                            int offset = nc - startC;
                            // reversed sequence:6,5,4,3,2,1 repeating left-to-right
                            int idx = (5 - (offset % 6) + 6) % 6;
                            wallTex = wallHallTex[idx];
                        }
                    }

                    shader.setBool("useTexture", true);
                    shader.setBool("useWorldUV", false);
                    shader.setVec2("texScale2", glm::vec2(1.0f, 1.0f));
                    // flip vertically only for hall textures to fix inverted pattern
                    shader.setBool("flipTexY", (t == 2) ? false : true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTex);
                    shader.setVec3("baseColor", glm::vec3(1.0f));

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x, 0.0f, w.z + TILE * 0.5f));
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    shader.setBool("useTexture", false);
                }

                // Sur (r+1)
                if (!InBounds(r + 1, c) || IsWall(r + 1, c)) {
                    int nr = r + 1;
                    int nc = c;
                    int t = InBounds(nr, nc) ? WallType(nr, nc) : 0;
                    unsigned int wallTex;
                    if (t == 2) wallTex = wallRoomTex;
                    else {
                        // check if this floor cell belongs to a marked indie horizontal segment
                        bool isIndie = false;
                        for (const auto& s : indieSegs) {
                            if (s.dir == 0 && s.startR == r && nc >= s.startC && nc < s.startC + s.len) { isIndie = true; break; }
                        }
                        if (isIndie) {
                            // find which tile of the pair this is and select appropriate indie texture
                            int chosen = 0;
                            for (const auto& s : indieSegs) {
                                if (s.dir == 0 && s.startR == r && nc >= s.startC && nc < s.startC + s.len) { chosen = nc - s.startC; break; }
                            }
                            wallTex = (chosen == 0) ? wallindie1 : wallindie2;
                        }
                        else {
                            // find start of continuous horizontal wall segment (scan left)
                            int startC = nc;
                            while (startC - 1 >= 0 && IsFloor(r, startC - 1) && IsWall(r + 1, startC - 1)) startC--;
                            int offset = nc - startC;
                            int idx = (5 - (offset % 6) + 6) % 6;
                            wallTex = wallHallTex[idx];
                        }
                    }

                    shader.setBool("useTexture", true);
                    shader.setBool("useWorldUV", false);
                    shader.setVec2("texScale2", glm::vec2(1.0f, 1.0f));
                    shader.setBool("flipTexY", (t == 2) ? false : true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTex);
                    shader.setVec3("baseColor", glm::vec3(1.0f));

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x, 0.0f, w.z - TILE * 0.5f));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    shader.setBool("useTexture", false);
                }

                // Oeste (c-1)
                if (!InBounds(r, c - 1) || IsWall(r, c - 1)) {
                    int nr = r;
                    int nc = c - 1;
                    int t = InBounds(nr, nc) ? WallType(nr, nc) : 0;
                    unsigned int wallTex;
                    if (t == 2) wallTex = wallRoomTex;
                    else {
                        // check if this floor cell belongs to a marked indie vertical segment
                        bool isIndie = false;
                        for (const auto& s : indieSegs) {
                            if (s.dir == 1 && s.startC == c && nr >= s.startR && nr < s.startR + s.len) { isIndie = true; break; }
                        }
                        if (isIndie) {
                            // find which tile of the pair this is and select appropriate indie texture
                            int chosen = 0;
                            for (const auto& s : indieSegs) {
                                if (s.dir == 1 && s.startC == c && nr >= s.startR && nr < s.startR + s.len) { chosen = nr - s.startR; break; }
                            }
                            wallTex = (chosen == 0) ? wallindie1 : wallindie2;
                        }
                        else {
                            // find start of continuous vertical wall segment (scan up)
                            int startR = nr;
                            while (startR - 1 >= 0 && IsFloor(startR - 1, c) && IsWall(startR - 1, c - 1)) startR--;
                            int offset = nr - startR;
                            int idx = (5 - (offset % 6) + 6) % 6;
                            wallTex = wallHallTex[idx];
                        }
                    }

                    shader.setBool("useTexture", true);
                    shader.setBool("useWorldUV", false);
                    shader.setVec2("texScale2", glm::vec2(1.0f, 1.0f));
                    shader.setBool("flipTexY", (t == 2) ? false : true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTex);
                    shader.setVec3("baseColor", glm::vec3(1.0f));

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x - TILE * 0.5f, 0.0f, w.z));
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0, 1, 0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    shader.setBool("useTexture", false);
                }

                // Este (c+1)
                if (!InBounds(r, c + 1) || IsWall(r, c + 1)) {
                    int nr = r;
                    int nc = c + 1;
                    int t = InBounds(nr, nc) ? WallType(nr, nc) : 0;
                    unsigned int wallTex;
                    if (t == 2) wallTex = wallRoomTex;
                    else {
                        // check if this floor cell belongs to a marked indie vertical segment
                        bool isIndie = false;
                        for (const auto& s : indieSegs) {
                            if (s.dir == 1 && s.startC == c && nr >= s.startR && nr < s.startR + s.len) { isIndie = true; break; }
                        }
                        if (isIndie) {
                            // find which tile of the pair this is and select appropriate indie texture
                            int chosen = 0;
                            for (const auto& s : indieSegs) {
                                if (s.dir == 1 && s.startC == c && nr >= s.startR && nr < s.startR + s.len) { chosen = nr - s.startR; break; }
                            }
                            wallTex = (chosen == 0) ? wallindie1 : wallindie2;
                        }
                        else {
                            // find start of continuous vertical wall segment (scan up)
                            int startR = nr;
                            while (startR - 1 >= 0 && IsFloor(startR - 1, c) && IsWall(startR - 1, c + 1)) startR--;
                            int offset = nr - startR;
                            int idx = (5 - (offset % 6) + 6) % 6;
                            wallTex = wallHallTex[idx];
                        }
                    }

                    shader.setBool("useTexture", true);
                    shader.setBool("useWorldUV", false);
                    shader.setVec2("texScale2", glm::vec2(1.0f, 1.0f));
                    shader.setBool("flipTexY", (t == 2) ? false : true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTex);
                    shader.setVec3("baseColor", glm::vec3(1.0f));

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(w.x + TILE * 0.5f, 0.0f, w.z));
                    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
                    model = glm::scale(model, glm::vec3(TILE, WALL_HEIGHT, 1.0f));
                    shader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    shader.setBool("useTexture", false);
                }

            }
        }

        // ================= ACTUALIZAR ENEMIGOS (IA con A*) =================
        // Algoritmo A* optimizado con persecución inteligente
        Point playerGrid = WorldToCell(camera.Position);

        Enemy* enemies[] = { &enemy1/*, &enemy2*/};
        for (Enemy* e : enemies) {
            // Usar A* optimizado con caché y persecución directa
            Point nextCell = GetNextStepAStar(*e, playerGrid.r, playerGrid.c, deltaTime);

            // 2. Obtener posición world del centro de esa casilla
            glm::vec3 targetWorld = CellToWorld(nextCell.r, nextCell.c);

            // 3. Moverse suavemente hacia ese objetivo
            glm::vec3 dir = targetWorld - e->pos;
            if (glm::length(dir) > 0.01f) {
                dir = glm::normalize(dir);
                e->pos += dir * e->speed * deltaTime;
                e->isMoving = true;

                // Actualizar tiempo de animación
                e->animTime += deltaTime * 5.0f;
                if (e->animTime > 6.28318f) e->animTime -= 6.28318f;
            }

            // SIEMPRE calcular la rotación hacia el jugador (no hacia donde se mueve)
            glm::vec3 dirToPlayer = camera.Position - e->pos;
            dirToPlayer.y = 0.0f;  // Ignorar diferencia de altura
            if (glm::length(dirToPlayer) > 0.01f) {
                dirToPlayer = glm::normalize(dirToPlayer);
                e->rotation = atan2(dirToPlayer.x, dirToPlayer.z);
            }

            // 4. Si está muy cerca del centro de la casilla objetivo, actualizar su grid lógico
            if (glm::distance(e->pos, targetWorld) < 0.1f) {
                e->r = nextCell.r;
                e->c = nextCell.c;
                // Remover el paso completado del caché
                if (!e->cachedPath.empty() && 
                    e->cachedPath.front().r == e->r && e->cachedPath.front().c == e->c) {
                    e->cachedPath.erase(e->cachedPath.begin());
                }
            }
        }

        // ================= DIBUJAR ENEMIGOS =================
        animator.UpdateAnimation(deltaTime);

        xenomorphShader.use();

        // Matrices básicas
        xenomorphShader.setMat4("projection", projection);
        xenomorphShader.setMat4("view", view);

        // Configuración de Luz
        xenomorphShader.setVec3("lightPos", lightPos);
        xenomorphShader.setVec3("viewPos", camera.Position);

        // Pasar las matrices de huesos al shader
        xenomorphShader.setBool("useAnimation", true);
        auto transforms = animator.GetFinalBoneMatrices();
        for (int i = 0; i < transforms.size() && i < 250; ++i) {
            xenomorphShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
        }

        for (Enemy* e : enemies) {
            glm::mat4 model = glm::mat4(1.0f);

            // Posición del enemigo
            glm::vec3 alturaAjustada = e->pos + glm::vec3(0.0f, 0.0f, 0.0f);
            model = glm::translate(model, alturaAjustada);

            // Rotar hacia la dirección de movimiento (necesitas agregar e->rotation)
            model = glm::rotate(model, e->rotation, glm::vec3(0.0f, 1.0f, 0.0f));

            // Escalar
            model = glm::scale(model, glm::vec3(1.3f));

            xenomorphShader.setMat4("model", model);
            xenomorphModel.Draw(xenomorphShader);
        }

        // ===== CUBO LUZ =====
        // The light cube visualization was removed per request so it is not rendered in the sky.
        // If you want to re-enable for debugging, uncomment the block below.
        /*
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);

        glm::mat4 mLight = glm::mat4(1.0f);
        mLight = glm::translate(mLight, lightPos);
        mLight = glm::scale(mLight, glm::vec3(LIGHT_CUBE_SCALE));
        lightShader.setMat4("model", mLight);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES,0,36);
        */

        // ================= RECOLECCIÓN Y DIBUJO DE CUBOS COLECCIONABLES (ANTORCHAS) =================
        CheckCollectibles(camera.Position, window);

        // ===== CONFIGURAR LUCES DE ANTORCHA =====
        // Las antorchas emiten luz naranja/amarilla tipo fuego
        {
            float t = (float)glfwGetTime();

            // Parpadeo de intensidad tipo fuego
            float flicker1 = sin(t * 15.0f) * 0.1f;
            float flicker2 = sin(t * 23.0f) * 0.08f;
            float flicker3 = sin(t * 7.0f) * 0.15f;
            float torchFlicker = 0.7f + flicker1 + flicker2 + flicker3;
            float torchIntensity = torchFlicker * 2.5f;

            // Color de fuego variable
            float colorShift = 0.5f + 0.5f * sin(t * 5.0f);
            glm::vec3 torchColor = glm::mix(
                glm::vec3(1.0f, 0.4f, 0.1f),  // Naranja
                glm::vec3(1.0f, 0.7f, 0.2f),  // Amarillo
                colorShift
            ) * torchIntensity;

            // Configurar hasta 8 luces de antorcha en el shader
            int torchIndex = 0;
            const int MAX_TORCH_LIGHTS = 8;

            for (const auto& col : collectibles) {
                if (col.collected || torchIndex >= MAX_TORCH_LIGHTS) continue;

                // Posición de luz ligeramente arriba del cubo
                glm::vec3 torchLightPos = col.pos + glm::vec3(0.0f, 0.3f, 0.0f);

                // Enviar al shader (nombre correcto: torchPositions)
                std::string uniform = "torchPositions[" + std::to_string(torchIndex) + "]";
                shader.setVec3(uniform, torchLightPos);
                torchIndex++;
            }

            // Informar cuántas antorchas hay activas
            shader.setInt("numTorchLights", torchIndex);
            shader.setVec3("torchColor", torchColor);
            shader.setFloat("torchIntensity", torchIntensity); // Intensidad variable
        }

        // ===== DIBUJAR ANTORCHAS CON EFECTO DE BRILLO =====
        if (!collectibles.empty()) {
            float t = (float)glfwGetTime();

            // Parpadeo de intensidad tipo fuego para el brillo visual
            float flicker1 = sin(t * 15.0f) * 0.1f;
            float flicker2 = sin(t * 23.0f) * 0.08f;
            float flicker3 = sin(t * 7.0f) * 0.15f;
            float torchFlicker = 0.85f + flicker1 + flicker2 + flicker3;

            // Pulso adicional
            float pulse = 0.85f + 0.15f * sin(t * 3.0f);

            // Color de fuego variable
            float colorShift = 0.5f + 0.5f * sin(t * 5.0f);
            glm::vec3 baseFireColor = glm::mix(
                glm::vec3(1.0f, 0.3f, 0.05f),  // Rojo-naranja
                glm::vec3(1.0f, 0.6f, 0.1f),   // Naranja-amarillo
                colorShift
            );

            // Color final con brillo intenso
            glm::vec3 torchEmissive = baseFireColor * torchFlicker * pulse * 3.0f;

            glBindVertexArray(cubeVAO);
            shader.use();
            shader.setBool("useTexture", false);

            for (const auto& col : collectibles) {
                if (col.collected) continue;

                // Movimiento sutil de la llama
                float yOffset = 0.03f * sin(t * 8.0f + col.pos.x * 2.0f);

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, col.pos + glm::vec3(0.0f, yOffset, 0.0f));

                // Escala variable para simular llama
                float scaleFlicker = 0.22f + 0.04f * sin(t * 12.0f + col.pos.z);
                model = glm::scale(model, glm::vec3(scaleFlicker));

                shader.setMat4("model", model);
                shader.setVec3("baseColor", torchEmissive);

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // ================= DIBUJAR MINIMAPA (solo si linterna encendida) =================
        if (linternaEncendida) {
            glDisable(GL_DEPTH_TEST);

            Point playerCell = WorldToCell(camera.Position);

            float mapX = SCR_WIDTH - MINIMAP_SIZE - MINIMAP_MARGIN;
            float mapY = SCR_HEIGHT - MINIMAP_SIZE - MINIMAP_MARGIN;

            glm::mat4 minimapProj = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);

            minimapShader.use();
            minimapShader.setMat4("projection", minimapProj);
            glBindVertexArray(minimapVAO);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Fondo del minimapa
            minimapShader.setVec3("color", glm::vec3(0.1f, 0.1f, 0.15f));
            minimapShader.setVec2("offset", glm::vec2(mapX - 5, mapY - 5));
            minimapShader.setVec2("scale", glm::vec2(MINIMAP_SIZE + 10, MINIMAP_SIZE + 10));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Dibujar celdas del minimapa
            for (int dr = -MINIMAP_RADIUS; dr <= MINIMAP_RADIUS; dr++) {
                for (int dc = -MINIMAP_RADIUS; dc <= MINIMAP_RADIUS; dc++) {
                    int r = playerCell.r + dr;
                    int c = playerCell.c + dc;

                    float cellX = mapX + (MINIMAP_RADIUS - dc) * MINIMAP_CELL_SIZE;
                    float cellY = mapY + (MINIMAP_RADIUS - dr) * MINIMAP_CELL_SIZE;

                    glm::vec3 cellColor;

                    if (!InBounds(r, c)) {
                        cellColor = glm::vec3(0.15f, 0.15f, 0.15f);
                    } else if (IsWall(r, c)) {
                        cellColor = glm::vec3(0.4f, 0.4f, 0.45f);
                    } else if (IsFloor(r, c)) {
                        cellColor = glm::vec3(0.2f, 0.15f, 0.1f);
                    } else if (Cell(r, c) == 'E') {
                        cellColor = glm::vec3(0.0f, 0.8f, 0.0f);
                    } else {
                        cellColor = glm::vec3(0.1f, 0.1f, 0.1f);
                    }

                    minimapShader.setVec3("color", cellColor);
                    minimapShader.setVec2("offset", glm::vec2(cellX, cellY));
                    minimapShader.setVec2("scale", glm::vec2(MINIMAP_CELL_SIZE - 1, MINIMAP_CELL_SIZE - 1));
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }

            // Dibujar jugador en el centro
            float playerX = mapX + MINIMAP_RADIUS * MINIMAP_CELL_SIZE;
            float playerY = mapY + MINIMAP_RADIUS * MINIMAP_CELL_SIZE;

            minimapShader.setVec3("color", glm::vec3(0.0f, 1.0f, 1.0f));
            minimapShader.setVec2("offset", glm::vec2(playerX + 2, playerY + 2));
            minimapShader.setVec2("scale", glm::vec2(MINIMAP_CELL_SIZE - 5, MINIMAP_CELL_SIZE - 5));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Borde del minimapa
            minimapShader.setVec3("color", glm::vec3(0.6f, 0.6f, 0.7f));
            minimapShader.setVec2("offset", glm::vec2(mapX - 5, mapY + MINIMAP_SIZE + 3));
            minimapShader.setVec2("scale", glm::vec2(MINIMAP_SIZE + 10, 3));
            glDrawArrays(GL_TRIANGLES, 0, 6);
            minimapShader.setVec2("offset", glm::vec2(mapX - 5, mapY - 8));
            minimapShader.setVec2("scale", glm::vec2(MINIMAP_SIZE + 10, 3));
            glDrawArrays(GL_TRIANGLES, 0, 6);
            minimapShader.setVec2("offset", glm::vec2(mapX - 8, mapY - 5));
            minimapShader.setVec2("scale", glm::vec2(3, MINIMAP_SIZE + 10));
            glDrawArrays(GL_TRIANGLES, 0, 6);
            minimapShader.setVec2("offset", glm::vec2(mapX + MINIMAP_SIZE + 3, mapY - 5));
            minimapShader.setVec2("scale", glm::vec2(3, MINIMAP_SIZE + 10));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}