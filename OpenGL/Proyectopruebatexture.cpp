
// Batches separados por textura
static std::vector<MazeBatch> mazeBatches;
static bool mazeBatchesInit = false;

// Cache de uniform locations para matrices de huesos
static std::vector<GLint> boneMatrixLocs;
static std::vector<GLint> torchPosLocs;
static bool uniformLocsInit = false;

// Inicializar buffers de pathfinding una sola vez
static void InitAStarBuffers() {
    InitPathfindBuffers();
}

// =====================================================================
// PRE-CÁLCULO DE TEXTURAS DE PAREDES (ejecutar una vez al inicio)
// =====================================================================
// Esto elimina los bucles while y búsquedas repetidas en cada frame

// Estructura para segmentos de textura indie (definida globalmente)
struct IndieSeg {
    int startR;
    int startC;
    int dir;  // dir:0=horizontal, 1=vertical
    int len;
};

static std::vector<IndieSeg> g_indieSegs; // Referencia global para indie segments

static void PrecomputeWallTextures() {
    if (wallTextureCacheInit) return;

    wallTextureCache.assign(MAP_H, std::vector<std::array<WallInfo, 4>>(MAP_W));

    for (int r = 0; r < MAP_H; r++) {
        for (int c = 0; c < MAP_W; c++) {
            if (!IsFloor(r, c)) continue;

            // Direcciones: 0=Norte, 1=Sur, 2=Oeste, 3=Este
            // Norte (r-1)
            if (!InBounds(r - 1, c) || IsWall(r - 1, c)) {
                int nr = r - 1, nc = c;
                int t = InBounds(nr, nc) ? WallType(nr, nc) : 0;
                WallInfo& info = wallTextureCache[r][c][0];

                if (t == 2) {
                    info.textureIndex = 6; // wallRoomTex
                    info.flipTexY = false;
                }
                else {
                    bool isIndie = false;
                    int chosen = 0;
                    for (const auto& s : g_indieSegs) {
                        if (s.dir == 0 && s.startR == r && nc >= s.startC && nc < s.startC + s.len) {
                            isIndie = true;
                            chosen = nc - s.startC;
                            break;
                        }
                    }
                    if (isIndie) {
                        info.textureIndex = (chosen == 0) ? 7 : 8; // wallindie1 o wallindie2
                    }
                    else {
                        int startC = nc;
                        while (startC - 1 >= 0 && IsFloor(r, startC - 1) && IsWall(r - 1, startC - 1)) startC--;
                        int offset = nc - startC;
                        info.textureIndex = (5 - (offset % 6) + 6) % 6; // wallHallTex[idx]
                    }
                    info.flipTexY = true;
                }
            }

            // Sur (r+1)
            if (!InBounds(r + 1, c) || IsWall(r + 1, c)) {
                int nr = r + 1, nc = c;
                int t = InBounds(nr, nc) ? WallType(nr, nc) : 0;
                WallInfo& info = wallTextureCache[r][c][1];

                if (t == 2) {
                    info.textureIndex = 6;
                    info.flipTexY = false;
                }
                else {
                    bool isIndie = false;
                    int chosen = 0;
                    for (const auto& s : g_indieSegs) {
                        if (s.dir == 0 && s.startR == r && nc >= s.startC && nc < s.startC + s.len) {
                            isIndie = true;
                            chosen = nc - s.startC;
                            break;
                        }
                    }
                    if (isIndie) {
                        info.textureIndex = (chosen == 0) ? 7 : 8;
                    }
                    else {
                        int startC = nc;
                        while (startC - 1 >= 0 && IsFloor(r, startC - 1) && IsWall(r + 1, startC - 1)) startC--;
                        int offset = nc - startC;
                        info.textureIndex = (5 - (offset % 6) + 6) % 6;
                    }
                    info.flipTexY = true;
                }
            }

            // Oeste (c-1)
            if (!InBounds(r, c - 1) || IsWall(r, c - 1)) {
                int nr = r, nc = c - 1;
                int t = InBounds(nr, nc) ? WallType(nr, nc) : 0;
                WallInfo& info = wallTextureCache[r][c][2];

                if (t == 2) {
                    info.textureIndex = 6;
                    info.flipTexY = false;
                }
                else {
                    bool isIndie = false;
                    int chosen = 0;
                    for (const auto& s : g_indieSegs) {
                        if (s.dir == 1 && s.startC == c && nr >= s.startR && nr < s.startR + s.len) {
                            isIndie = true;
                            chosen = nr - s.startR;
                            break;
                        }
                    }
                    if (isIndie) {
                        info.textureIndex = (chosen == 0) ? 7 : 8;
                    }
                    else {
                        int startR = nr;
                        while (startR - 1 >= 0 && IsFloor(startR - 1, c) && IsWall(startR - 1, c - 1)) startR--;
                        int offset = nr - startR;
                        info.textureIndex = (5 - (offset % 6) + 6) % 6;
                    }
                    info.flipTexY = true;
                }
            }

            // Este (c+1)
            if (!InBounds(r, c + 1) || IsWall(r, c + 1)) {
                int nr = r, nc = c + 1;
                int t = InBounds(nr, nc) ? WallType(nr, nc) : 0;
                WallInfo& info = wallTextureCache[r][c][3];

                if (t == 2) {
                    info.textureIndex = 6;
                    info.flipTexY = false;
                }
                else {
                    bool isIndie = false;
                    int chosen = 0;
                    for (const auto& s : g_indieSegs) {
                        if (s.dir == 1 && s.startC == c && nr >= s.startR && nr < s.startR + s.len) {
                            isIndie = true;
                            chosen = nr - s.startR;
                            break;
                        }
                    }
                    if (isIndie) {
                        info.textureIndex = (chosen == 0) ? 7 : 8;
                    }
                    else {
                        int startR = nr;
                        while (startR - 1 >= 0 && IsFloor(startR - 1, c) && IsWall(startR - 1, c + 1)) startR--;
                        int offset = nr - startR;
                        info.textureIndex = (5 - (offset % 6) + 6) % 6;
                    }
                    info.flipTexY = true;
                }
            }
        }
    }

    wallTextureCacheInit = true;
    std::cout << "Wall texture cache inicializado para " << MAP_H << "x" << MAP_W << " celdas\n";
}

// =====================================================================
// BATCHING DE GEOMETRÍA DEL LABERINTO
// =====================================================================
// Agrupa toda la geometría por textura para reducir draw calls masivamente

// Array global de texturas (se llenará en main)
static unsigned int g_allTextures[9]; // 0-5: hall, 6: room, 7-8: indie, + floor/ceiling

static void BuildMazeGeometry() {
    if (mazeBatchesInit) return;

    // Limpiar batches anteriores
    for (auto& batch : mazeBatches) {
        if (batch.VAO) glDeleteVertexArrays(1, &batch.VAO);
        if (batch.VBO) glDeleteBuffers(1, &batch.VBO);
    }
    mazeBatches.clear();

    // Recopilar vértices por textura (índices 0-8 para paredes, 9 para suelo, 10 para techo)
    std::vector<std::vector<float>> verticesByTexture(11);

    // Función lambda para agregar un quad de suelo/techo
    // Genera un quad horizontal en la posición (x, z) a altura y
    auto addFloorQuad = [](std::vector<float>& verts, float x, float z, float y, float nx, float ny, float nz) {
        float halfT = TILE * 0.5f;

        // Esquinas del quad (visto desde arriba)
        // Para suelo (ny > 0): normal apunta hacia arriba
        // Para techo (ny < 0): normal apunta hacia abajo

        // Triángulo 1: esquina inferior-izquierda -> superior-izquierda -> superior-derecha
        verts.insert(verts.end(), { x - halfT, y, z - halfT, nx, ny, nz, 0.0f, 0.0f });
        verts.insert(verts.end(), { x + halfT, y, z - halfT, nx, ny, nz, 1.0f, 0.0f });
        verts.insert(verts.end(), { x + halfT, y, z + halfT, nx, ny, nz, 1.0f, 1.0f });

        // Triángulo 2
        verts.insert(verts.end(), { x + halfT, y, z + halfT, nx, ny, nz, 1.0f, 1.0f });
        verts.insert(verts.end(), { x - halfT, y, z + halfT, nx, ny, nz, 0.0f, 1.0f });
        verts.insert(verts.end(), { x - halfT, y, z - halfT, nx, ny, nz, 0.0f, 0.0f });
        };

    // Función lambda para agregar un quad de pared
    // Replica exactamente el orden y UVs del wallVertices original
    // wallVertices original: UV(0,1) en bottom-left, UV(1,0) en top-right
    // Con flipY=true: invertir las V (para texturas hall)
    auto addWallQuad = [](std::vector<float>& verts, glm::vec3 pos, float rotY, float w, float h, glm::vec3 normal, bool flipY) {
        float halfW = w * 0.5f;
        float cosR = cos(rotY), sinR = sin(rotY);

        // Vector "right" en el plano XZ (perpendicular a la normal)
        glm::vec3 right = glm::vec3(cosR, 0.0f, sinR) * halfW;

        // Las 4 esquinas del quad
        glm::vec3 bl = pos - right;                          // Bottom-left
        glm::vec3 br = pos + right;                          // Bottom-right
        glm::vec3 tl = bl + glm::vec3(0.0f, h, 0.0f);        // Top-left
        glm::vec3 tr = br + glm::vec3(0.0f, h, 0.0f);        // Top-right

        // Coordenadas UV - orden original del wallVertices:
        // bottom-left = (0, 1), bottom-right = (1, 1)
        // top-left = (0, 0), top-right = (1, 0)
        // Si flipY=true, invertimos V: bottom usa V=0, top usa V=1
        float vBottom = flipY ? 0.0f : 1.0f;
        float vTop = flipY ? 1.0f : 0.0f;

        // Triángulo 1: bl -> br -> tr (igual que wallVertices)
        verts.insert(verts.end(), { bl.x, bl.y, bl.z, normal.x, normal.y, normal.z, 0.0f, vBottom });
        verts.insert(verts.end(), { br.x, br.y, br.z, normal.x, normal.y, normal.z, 1.0f, vBottom });
        verts.insert(verts.end(), { tr.x, tr.y, tr.z, normal.x, normal.y, normal.z, 1.0f, vTop });

        // Triángulo 2: tr -> tl -> bl (igual que wallVertices)
        verts.insert(verts.end(), { tr.x, tr.y, tr.z, normal.x, normal.y, normal.z, 1.0f, vTop });
        verts.insert(verts.end(), { tl.x, tl.y, tl.z, normal.x, normal.y, normal.z, 0.0f, vTop });
        verts.insert(verts.end(), { bl.x, bl.y, bl.z, normal.x, normal.y, normal.z, 0.0f, vBottom });
        };

    // Recorrer todo el mapa y construir geometría
    for (int r = 0; r < MAP_H; r++) {
        for (int c = 0; c < MAP_W; c++) {
            if (!IsFloor(r, c)) continue;

            glm::vec3 w = CellToWorld(r, c);

            // Suelo (textura índice 9)
            addFloorQuad(verticesByTexture[9], w.x, w.z, 0.0f, 0.0f, 1.0f, 0.0f);

            // Techo (textura índice 10 - sin textura, color sólido)
            addFloorQuad(verticesByTexture[10], w.x, w.z, WALL_HEIGHT, 0.0f, -1.0f, 0.0f);

            // Paredes Norte (dir 0) - mirando hacia -Z (dentro de la celda)
            if (!InBounds(r - 1, c) || IsWall(r - 1, c)) {
                const WallInfo& info = wallTextureCache[r][c][0];
                glm::vec3 wallPos = glm::vec3(w.x, 0.0f, w.z + TILE * 0.5f);
                // Rotación 180° => la pared mira hacia -Z
                addWallQuad(verticesByTexture[info.textureIndex], wallPos,
                    glm::radians(0.0f), TILE, WALL_HEIGHT,
                    glm::vec3(0.0f, 0.0f, -1.0f), info.flipTexY);
            }

            // Paredes Sur (dir 1) - mirando hacia +Z (dentro de la celda)
            if (!InBounds(r + 1, c) || IsWall(r + 1, c)) {
                const WallInfo& info = wallTextureCache[r][c][1];
                glm::vec3 wallPos = glm::vec3(w.x, 0.0f, w.z - TILE * 0.5f);
                // Sin rotación => la pared mira hacia +Z
                addWallQuad(verticesByTexture[info.textureIndex], wallPos,
                    glm::radians(180.0f), TILE, WALL_HEIGHT,
                    glm::vec3(0.0f, 0.0f, 1.0f), info.flipTexY);
            }

            // Paredes Oeste (dir 2) - mirando hacia +X (dentro de la celda)
            if (!InBounds(r, c - 1) || IsWall(r, c - 1)) {
                const WallInfo& info = wallTextureCache[r][c][2];
                glm::vec3 wallPos = glm::vec3(w.x - TILE * 0.5f, 0.0f, w.z);
                // Rotación -90° => la pared mira hacia +X
                addWallQuad(verticesByTexture[info.textureIndex], wallPos,
                    glm::radians(90.0f), TILE, WALL_HEIGHT,
                    glm::vec3(1.0f, 0.0f, 0.0f), info.flipTexY);
            }

            // Paredes Este (dir 3) - mirando hacia -X (dentro de la celda)
            if (!InBounds(r, c + 1) || IsWall(r, c + 1)) {
                const WallInfo& info = wallTextureCache[r][c][3];
                glm::vec3 wallPos = glm::vec3(w.x + TILE * 0.5f, 0.0f, w.z);
                // Rotación 90° => la pared mira hacia -X
                addWallQuad(verticesByTexture[info.textureIndex], wallPos,
                    glm::radians(-90.0f), TILE, WALL_HEIGHT,
                    glm::vec3(-1.0f, 0.0f, 0.0f), info.flipTexY);
            }
        }
    }

    // Crear VAO/VBO para cada grupo de texturas
    for (int i = 0; i < 11; i++) {
        if (verticesByTexture[i].empty()) continue;

        MazeBatch batch;
        batch.vertexCount = (int)verticesByTexture[i].size() / 8; // 8 floats per vertex
        batch.textureID = (i < 9) ? g_allTextures[i] : 0; // Para suelo/techo se manejará aparte

        glGenVertexArrays(1, &batch.VAO);
        glGenBuffers(1, &batch.VBO);

        glBindVertexArray(batch.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, batch.VBO);
        glBufferData(GL_ARRAY_BUFFER, verticesByTexture[i].size() * sizeof(float),
            verticesByTexture[i].data(), GL_STATIC_DRAW);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // texcoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        glBindVertexArray(0);

        mazeBatches.push_back(batch);

        std::cout << "Batch " << i << ": " << batch.vertexCount << " vertices\n";
    }

    mazeBatchesInit = true;
    std::cout << "Maze geometry batched: " << mazeBatches.size() << " batches (vs ~"
        << (MAP_H * MAP_W * 6) << " draw calls antes)\n";
}

// Función para inicializar cache de uniform locations
static void InitUniformLocations(unsigned int xenomorphShaderID) {
    if (uniformLocsInit) return;

    boneMatrixLocs.resize(250);
    for (int i = 0; i < 250; i++) {
        std::string name = "finalBonesMatrices[" + std::to_string(i) + "]";
        boneMatrixLocs[i] = glGetUniformLocation(xenomorphShaderID, name.c_str());
    }

    torchPosLocs.resize(8);
    for (int i = 0; i < 8; i++) {
        std::string name = "torchPositions[" + std::to_string(i) + "]";
        torchPosLocs[i] = glGetUniformLocation(xenomorphShaderID, name.c_str());
    }

    uniformLocsInit = true;
    std::cout << "Uniform locations cacheados\n";
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
                gameWin = true;   // <<<<<< AQUÍ SE GANA
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
// ================= RESET DEL JUEGO =================
void ResetGame() {
    // Reset flags
    gameOver = false;
    gameWin = false;
    gameOver = false;
    // Reset tiempo (evita deltaTime gigante)
    lastFrame = (float)glfwGetTime();

    // ===== RESET CAMARA / JUGADOR =====
    int sr = 0, sc = 0;
    if (FindSpawn(sr, sc)) {
        glm::vec3 spawnW = CellToWorld(sr, sc);
        camera.Position = glm::vec3(spawnW.x, 2.0f, spawnW.z);
        camera.Yaw = 180.0f;
        camera.Pitch = 0.0f;
        camera.ProcessMouseMovement(0, 0);
    }

    // ===== RESET ENEMIGOS =====
    SpawnEnemies(camera.Position);

    // ===== RESET COLECCIONABLES =====
    SpawnCollectibles(camera.Position);
}

}
=========
>>>>>>>>> Temporary merge branch 2

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
    gameOverTex = loadTexture("textures/gameover.png");
    gameWinTex = loadTexture("textures/win.png"); 


<<<<<<<<< Temporary merge branch 1
    gameOverTex = loadTexture("textures/gameover.png");
    gameWinTex = loadTexture("textures/win.png"); 


=========
>>>>>>>>> Temporary merge branch 2
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
    // IndieSeg ya está definido globalmente
    // Usar vector global para que PrecomputeWallTextures pueda acceder
    g_indieSegs.clear();
    const int MAX_INDIE = 4;
    const int NARROW_THRESHOLD = 2; // longitud de segmento <= umbral considerado "estrecho"

    for (int rr = 0; rr < MAP_H && (int)g_indieSegs.size() < MAX_INDIE; rr++) {
        for (int cc = 0; cc < MAP_W && (int)g_indieSegs.size() < MAX_INDIE; cc++) {
            if (!IsFloor(rr, cc)) continue;
            // pared horizontal sobre esta celda de suelo (facing north)
            if (!InBounds(rr - 1, cc) || IsWall(rr - 1, cc)) {
                int startC = cc;
                while (startC - 1 >= 0 && IsFloor(rr, startC - 1) && (!InBounds(rr - 1, startC - 1) || IsWall(rr - 1, startC - 1))) startC--;
                int len = 0;
                int c2 = startC;
                while (c2 < MAP_W && IsFloor(rr, c2) && (!InBounds(rr - 1, c2) || IsWall(rr - 1, c2))) { len++; c2++; }
                // only mark segments that are exactly NARROW_THRESHOLD long (pairs)
                if (len == NARROW_THRESHOLD) g_indieSegs.push_back({ rr, startC,0, len });
            }
            if ((int)g_indieSegs.size() >= MAX_INDIE) break;

            // pared vertical a la izquierda de esta celda de suelo (facing west)
            if (!InBounds(rr, cc - 1) || IsWall(rr, cc - 1)) {
                int startR = rr;
                while (startR - 1 >= 0 && IsFloor(startR - 1, cc) && (!InBounds(startR - 1, cc - 1) || IsWall(startR - 1, cc - 1))) startR--;
                int len = 0;
                int r2 = startR;
                while (r2 < MAP_H && IsFloor(r2, cc) && (!InBounds(r2, cc - 1) || IsWall(r2, cc - 1))) { len++; r2++; }
                // only mark segments that are exactly NARROW_THRESHOLD long (pairs)
                if (len == NARROW_THRESHOLD) g_indieSegs.push_back({ startR, cc,1, len });
            }
        }
    }

    // Asegurar que el shader use la unidad de textura0 para 'texture1'
    shader.use();
    shader.setInt("texture1", 0);

    // ===== INICIALIZAR ARRAY GLOBAL DE TEXTURAS PARA BATCHING =====
    g_allTextures[0] = wallHallTex[0];
    g_allTextures[1] = wallHallTex[1];
    g_allTextures[2] = wallHallTex[2];
    g_allTextures[3] = wallHallTex[3];
    g_allTextures[4] = wallHallTex[4];
    g_allTextures[5] = wallHallTex[5];
  
    g_allTextures[7] = wallindie1;
    g_allTextures[8] = wallindie2;

    // ===== PRE-CALCULAR TEXTURAS Y GEOMETRÍA DEL LABERINTO =====
    PrecomputeWallTextures();
    BuildMazeGeometry();
<<<<<<<<< Temporary merge branch 1
  
=========
>>>>>>>>> Temporary merge branch 2

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
    // Configurar posición y orientación de la cámara
    camera.Position = glm::vec3(spawnW.x, 2.0f, spawnW.z);
    camera.Yaw = 180.0f;
    camera.Pitch = 0.0f;
    camera.ProcessMouseMovement(0, 0); // Actualizar vectores


    // === NUEVO: INICIALIZAR ENEMIGOS ===
    srand((unsigned int)glfwGetTime()); // Semilla random
    SpawnEnemies(camera.Position);

    // USAR UN SHADER ESTÁNDAR PARA MODELOS 3D
    // (Asegúrate de tener "model_loading.vs" y "model_loading.fs" en tu carpeta shaders,
    //  son los shaders por defecto de LearnOpenGL para modelos).
    Shader xenomorphShader("shaders/model_loading.vs", "shaders/model_loading.fs");

    // ===== CACHEAR UNIFORM LOCATIONS PARA OPTIMIZACIÓN =====
    InitUniformLocations(xenomorphShader.ID);

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
            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);

            glfwSwapBuffers(window);
            continue;
        }
        // ===== GAME OVER =====
        if (gameOver) {

            // ---- REINICIAR CON ENTER ----
            static bool enterPrev = false;
            bool enter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;

            if (enter && !enterPrev) {
                ResetGame();   // <<<<<< AQUÍ SE RESETEA TODO
            }
            enterPrev = enter;

            // ---- DIBUJAR PANTALLA GAME OVER ----
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            screenShader.use();
            glBindVertexArray(screenVAO);

            glDisable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gameOverTex);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            glEnable(GL_DEPTH_TEST);

            glfwSwapBuffers(window);
            continue;   // CRÍTICO
        }
        // ===== GAME WIN =====
        if (gameWin) {

            // ---- REINICIAR CON ENTER ----
            static bool enterPrevWin = false;
            bool enter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;

            if (enter && !enterPrevWin) {
                ResetGame();   // mismo reset que Game Over
                gameWin = false;
            }
            enterPrevWin = enter;

            // ---- DIBUJAR PANTALLA WIN ----
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            screenShader.use();
            glBindVertexArray(screenVAO);

                gameWin = false;
            }
            enterPrevWin = enter;

            // ---- DIBUJAR PANTALLA WIN ----
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);




            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);

            glfwSwapBuffers(window);
            continue;
        }



=========
>>>>>>>>> Temporary merge branch 2

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

        // ===== CONFIGURAR LUCES DE ANTORCHA ANTES DE DIBUJAR EL LABERINTO =====
        // Las antorchas emiten luz naranja/amarilla tipo fuego
        {
            float t = (float)glfwGetTime();

            // Parpadeo de intensidad tipo fuego
            float flicker1 = sin(t * 15.0f) * 0.1f;
            float flicker2 = sin(t * 23.0f) * 0.08f;
            float flicker3 = sin(t * 7.0f) * 0.15f;
            float torchFlicker = 0.7f + flicker1 + flicker2 + flicker3;
            float torchIntensity = torchFlicker * 1.5f; // Reducido de 2.5f a 1.5f

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

                // Enviar al shader
                std::string uniform = "torchPositions[" + std::to_string(torchIndex) + "]";
                shader.setVec3(uniform, torchLightPos);
                torchIndex++;
            }

            // Informar cuántas antorchas hay activas
            shader.setInt("numTorchLights", torchIndex);
            shader.setVec3("torchColor", torchColor);
            shader.setFloat("torchIntensity", torchIntensity);
        }

        // ===== DIBUJAR LABERINTO OPTIMIZADO (BATCHING) =====
        // Usar geometría pre-calculada: ~10 draw calls en lugar de ~100,000
        {
            glm::mat4 identity = glm::mat4(1.0f);
            shader.setMat4("model", identity);

            int batchIdx = 0;
            for (const auto& batch : mazeBatches) {
                glBindVertexArray(batch.VAO);

                // Configurar textura según el tipo de batch
                if (batchIdx < 9) {
                    // Paredes con textura
                    shader.setBool("useTexture", true);
                    shader.setBool("useWorldUV", false);
                    shader.setVec2("texScale2", glm::vec2(1.0f, 1.0f));
                    shader.setBool("flipTexY", false); // Ya se manejó en BuildMazeGeometry
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, batch.textureID);
                    shader.setVec3("baseColor", glm::vec3(1.0f));
                }
                else if (batchIdx == 9) {
                    // Suelo
                    glActiveTexture(GL_TEXTURE0);
                glDrawArrays(GL_TRIANGLES, 0, batch.vertexCount);
                batchIdx++;
            }

            }
            
>>>>>>>>> Temporary merge branch 2
            }
            
>>>>>>>>> Temporary merge branch 2
                    // Techo (sin textura)
                    shader.setBool("useTexture", false);
                    shader.setVec3("baseColor", glm::vec3(0.25f, 0.25f, 0.25f));
                }
<<<<<<<<< Temporary merge branch 1

                glDrawArrays(GL_TRIANGLES, 0, batch.vertexCount);
                batchIdx++;
            }

=========
                
                glDrawArrays(GL_TRIANGLES, 0, batch.vertexCount);
                batchIdx++;
            }
            
>>>>>>>>> Temporary merge branch 2
            shader.setBool("useTexture", false);
        }

        // ================= ACTUALIZAR ENEMIGOS (IA con BFS) =================
        // Algoritmo BFS para pathfinding
        Point playerGrid = WorldToCell(camera.Position);

        Enemy* enemies[] = { &enemy1/*, &enemy2*/ };
        for (Enemy* e : enemies) {
            for (Enemy* e : enemies) {
                float d = glm::distance(
                    glm::vec2(camera.Position.x, camera.Position.z),
                    glm::vec2(e->pos.x, e->pos.z)
                );
            // Usar BFS para encontrar el siguiente paso
                if (d < ENEMY_KILL_RADIUS) {
                    gameOver = true;
                    break;
                }
            }

            Point nextCell = GetNextStepBFS(e->r, e->c, playerGrid.r, playerGrid.c);

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

        // ===== PASAR LINTERNA AL SHADER DEL XENOMORPH =====
        xenomorphShader.setBool("linterna", linternaEncendida);
        if (linternaEncendida) {
            xenomorphShader.setVec3("spotLightPos", camera.Position);
            xenomorphShader.setVec3("spotLightDir", camera.Front);
            xenomorphShader.setFloat("spotCutOff", glm::cos(glm::radians(15.0f)));
            xenomorphShader.setFloat("spotOuterCutOff", glm::cos(glm::radians(25.0f)));
        }
        else {
            xenomorphShader.setVec3("spotLightPos", glm::vec3(0.0f));
            xenomorphShader.setVec3("spotLightDir", glm::vec3(0.0f, 0.0f, -1.0f));
            xenomorphShader.setFloat("spotCutOff", 0.0f);
            xenomorphShader.setFloat("spotOuterCutOff", 0.0f);
        }

        // Pasar las matrices de huesos al shader (OPTIMIZADO: usando uniform locations cacheados)
        xenomorphShader.setBool("useAnimation", true);
        auto transforms = animator.GetFinalBoneMatrices();
        for (int i = 0; i < (int)transforms.size() && i < 250; ++i) {
            if (boneMatrixLocs[i] >= 0) {
                glUniformMatrix4fv(boneMatrixLocs[i], 1, GL_FALSE, glm::value_ptr(transforms[i]));
            }
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

        // ================= RECOLECCIÓN DE CUBOS COLECCIONABLES =================
        CheckCollectibles(camera.Position, window);

        // ===== DIBUJAR ANTORCHAS CON EFECTO DE BRILLO MEJORADO =====
        if (!collectibles.empty()) {
            float t = (float)glfwGetTime();

            glBindVertexArray(cubeVAO);
            shader.use();
            shader.setBool("useTexture", false);

            // Habilitar blending para efectos de glow
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Blending aditivo para brillo

            for (const auto& col : collectibles) {
                if (col.collected) continue;

                // Parpadeo de intensidad tipo fuego
                float flicker1 = sin(t * 15.0f + col.pos.x) * 0.1f;
                float flicker2 = sin(t * 23.0f + col.pos.z) * 0.08f;
                float flicker3 = sin(t * 7.0f + col.pos.x * col.pos.z) * 0.15f;
                float torchFlicker = 0.85f + flicker1 + flicker2 + flicker3;

                // Pulso adicional
                float pulse = 0.85f + 0.15f * sin(t * 3.0f + col.pos.x);

                // Color de fuego variable
                float colorShift = 0.5f + 0.5f * sin(t * 5.0f + col.pos.z * 0.5f);
                glm::vec3 baseFireColor = glm::mix(
                    glm::vec3(1.0f, 0.3f, 0.05f),  // Rojo-naranja
                    glm::vec3(1.0f, 0.6f, 0.1f),   // Naranja-amarillo
                    colorShift
                );

                // Movimiento sutil de la llama
                float yOffset = 0.05f * sin(t * 8.0f + col.pos.x * 2.0f);
                float xOffset = 0.02f * sin(t * 6.0f + col.pos.z);
                float zOffset = 0.02f * cos(t * 7.0f + col.pos.x);

                // === CAPA 1: Núcleo brillante (centro de la llama) ===
                {
                    glm::vec3 coreColor = glm::vec3(1.0f, 0.9f, 0.5f) * torchFlicker * pulse * 2.0f;
                    float coreScale = 0.12f + 0.02f * sin(t * 20.0f);

                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, col.pos + glm::vec3(xOffset, yOffset + 0.1f, zOffset));
                    model = glm::scale(model, glm::vec3(coreScale));

                    shader.setMat4("model", model);
                    shader.setVec3("baseColor", coreColor);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // === CAPA 2: Llama principal ===
                {
                    glm::vec3 flameColor = baseFireColor * torchFlicker * pulse * 1.5f;
                    float flameScale = 0.18f + 0.03f * sin(t * 12.0f + col.pos.z);

                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, col.pos + glm::vec3(xOffset * 0.5f, yOffset, zOffset * 0.5f));
                    model = glm::scale(model, glm::vec3(flameScale));

                    shader.setMat4("model", model);
                    shader.setVec3("baseColor", flameColor);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // === CAPA 3: Resplandor exterior (glow) ===
                {
                    glm::vec3 glowColor = glm::vec3(1.0f, 0.4f, 0.1f) * torchFlicker * 0.4f;
                    float glowScale = 0.28f + 0.04f * sin(t * 8.0f);

                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, col.pos + glm::vec3(0.0f, yOffset * 0.5f, 0.0f));
                    model = glm::scale(model, glm::vec3(glowScale));

                    shader.setMat4("model", model);
                    shader.setVec3("baseColor", glowColor);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // === CAPA 4: Halo difuso (atmosfera) ===
                {
                    glm::vec3 haloColor = glm::vec3(1.0f, 0.3f, 0.05f) * torchFlicker * 0.15f;
                    float haloScale = 0.4f + 0.08f * sin(t * 4.0f);

                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, col.pos);
                    model = glm::scale(model, glm::vec3(haloScale));

                    shader.setMat4("model", model);
                    shader.setVec3("baseColor", haloColor);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // === PARTÍCULAS DE CHISPAS (pequeños cubos que suben) ===
                for (int spark = 0; spark < 3; spark++) {
                    float sparkPhase = t * 2.0f + spark * 2.1f + col.pos.x;
                    float sparkY = fmod(sparkPhase, 1.5f); // Ciclo de 0 a 1.5
                    float sparkLife = 1.0f - (sparkY / 1.5f); // Fade out mientras sube

                    if (sparkLife > 0.0f) {
                        float sparkX = sin(sparkPhase * 3.0f + spark) * 0.15f;
                        float sparkZ = cos(sparkPhase * 2.5f + spark * 0.7f) * 0.15f;

                        glm::vec3 sparkColor = glm::vec3(1.0f, 0.7f, 0.2f) * sparkLife * torchFlicker * 1.0f;
                        float sparkScale = 0.03f * sparkLife;

                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, col.pos + glm::vec3(sparkX, sparkY + 0.2f, sparkZ));
                        model = glm::scale(model, glm::vec3(sparkScale));

                        shader.setMat4("model", model);
                        shader.setVec3("baseColor", sparkColor);
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }
            }

            // Restaurar blending normal
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_BLEND);
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
                    }
                    else if (IsWall(r, c)) {
                        cellColor = glm::vec3(0.4f, 0.4f, 0.45f);
                    }
                    else if (IsFloor(r, c)) {
                        cellColor = glm::vec3(0.2f, 0.15f, 0.1f);
                    }
                    else if (Cell(r, c) == 'E') {
                        cellColor = glm::vec3(0.0f, 0.8f, 0.0f);
                    }
                    else {
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
        // glfwPollEvents() ya se llama al inicio del loop, no duplicar aquí
    }

    glfwTerminate();
    return 0;
}