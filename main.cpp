#include "raylib.h"
#include <cmath>
#include <vector>
#include <algorithm>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;
const float FOV = 60.0f * DEG2RAD;
const float BLOCK_SIZE = 64.0f;
const int NUM_RAYS = SCREEN_WIDTH;
const int SPRITE_SIZE = 32; // Tamaño de la textura del sprite

int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,2,0,0,3,0,1},
    {1,0,0,0,4,0,0,1}, // 4 = cubo morado (objetivo)
    {1,0,2,0,0,3,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1}
};

struct Player {
    float x, y;        
    float angle;       
    bool hasWon;       
};

struct Sprite {
    float x, y;
    int type; // 0 = cubo azul, 1 = cubo verde, 2 = cubo rojo
    bool active;
    float distance; // Para ordenamiento por profundidad
};

struct Intersect {
    float distance;
    char impact;
};

// Array de sprites en el mundo
std::vector<Sprite> sprites;

// Buffer de profundidad para sprites (z-buffer)
float* depthBuffer;

Color* CreateCubeTexture(Color cubeColor) {
    Color* texture = new Color[SPRITE_SIZE * SPRITE_SIZE];
    
    for (int y = 0; y < SPRITE_SIZE; y++) {
        for (int x = 0; x < SPRITE_SIZE; x++) {
            // Crear un cubo simple con bordes más oscuros
            if (x < 2 || x >= SPRITE_SIZE-2 || y < 2 || y >= SPRITE_SIZE-2) {
                // Bordes oscuros
                texture[y * SPRITE_SIZE + x] = {
                    (unsigned char)(cubeColor.r * 0.3f),
                    (unsigned char)(cubeColor.g * 0.3f),
                    (unsigned char)(cubeColor.b * 0.3f),
                    255
                };
            } else if (x < 4 || x >= SPRITE_SIZE-4 || y < 4 || y >= SPRITE_SIZE-4) {
                // Bordes medios
                texture[y * SPRITE_SIZE + x] = {
                    (unsigned char)(cubeColor.r * 0.7f),
                    (unsigned char)(cubeColor.g * 0.7f),
                    (unsigned char)(cubeColor.b * 0.7f),
                    255
                };
            } else {
                // Centro del cubo
                texture[y * SPRITE_SIZE + x] = cubeColor;
            }
        }
    }
    
    return texture;
}

// Texturas de sprites
Color* spriteTextures[3];

bool IsWall(float x, float y) {
    int mapX = (int)x;
    int mapY = (int)y;
    
    if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
        return true;
    }
    
    return worldMap[mapY][mapX] == 1;
}

Intersect CastRay(float startX, float startY, float angle, float blockSize, bool drawLine = false) {
    float d = 0.0f;
    
    while (true) {
        float cosA = d * cosf(angle);
        float sinA = d * sinf(angle);
        float x = startX + cosA;
        float y = startY + sinA;
        
        int i = (int)(x / blockSize);
        int j = (int)(y / blockSize);
        
        if (i < 0 || i >= MAP_WIDTH || j < 0 || j >= MAP_HEIGHT) {
            return {d, '1'};
        }
        
        if (worldMap[j][i] != 0) {
            if (worldMap[j][i] == 4) {
                return {d, (char)('0' + worldMap[j][i])};
            }
            return {d, (char)('0' + worldMap[j][i])};
        }
        
        if (drawLine) {
            DrawPixel((int)x * 60 / MAP_WIDTH, (int)y * 60 / MAP_HEIGHT, YELLOW);
        }
        
        d += 5.0f; 
    }
}

void DrawSprite(Player& player, Sprite& sprite, Color* texture) {
    // Calcular vector del jugador al sprite
    float spriteX = sprite.x - player.x;
    float spriteY = sprite.y - player.y;
    
    // Transformar coordenadas del sprite al espacio de la cámara
    float invDet = 1.0f / (cosf(player.angle + PI/2) * cosf(player.angle) - sinf(player.angle + PI/2) * sinf(player.angle));
    float transformX = invDet * (cosf(player.angle) * spriteX - sinf(player.angle) * spriteY);
    float transformY = invDet * (-sinf(player.angle + PI/2) * spriteX + cosf(player.angle + PI/2) * spriteY);
    
    // Si el sprite está detrás del jugador, no dibujarlo
    if (transformY <= 0) return;
    
    // Calcular posición en pantalla
    int spriteScreenX = (int)((SCREEN_WIDTH / 2) * (1 + transformX / transformY));
    int spriteHeight = abs((int)(SCREEN_HEIGHT / transformY));
    int spriteWidth = spriteHeight; 
    
    // Calcular límites de dibujo
    int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
    if (drawStartY < 0) drawStartY = 0;
    int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
    if (drawEndY >= SCREEN_HEIGHT) drawEndY = SCREEN_HEIGHT - 1;
    
    int drawStartX = -spriteWidth / 2 + spriteScreenX;
    if (drawStartX < 0) drawStartX = 0;
    int drawEndX = spriteWidth / 2 + spriteScreenX;
    if (drawEndX >= SCREEN_WIDTH) drawEndX = SCREEN_WIDTH - 1;
    
    for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
        int texX = (int)(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * SPRITE_SIZE / spriteWidth) / 256;
        
        // Solo dibujar si el sprite está más cerca que la pared
        if (transformY < depthBuffer[stripe]) {
            for (int y = drawStartY; y < drawEndY; y++) {
                int d = (y) * 256 - SCREEN_HEIGHT * 128 + spriteHeight * 128;
                int texY = ((d * SPRITE_SIZE) / spriteHeight) / 256;
                
                if (texX >= 0 && texX < SPRITE_SIZE && texY >= 0 && texY < SPRITE_SIZE) {
                    Color color = texture[texY * SPRITE_SIZE + texX];
                    
                    // No dibujar píxeles transparentes (negros en este caso)
                    if (color.r > 10 || color.g > 10 || color.b > 10) {
                        // Aplicar sombreado por distancia
                        float brightness = 1.0f / (1 + transformY * 0.01f);
                        color.r = (unsigned char)(color.r * brightness);
                        color.g = (unsigned char)(color.g * brightness);
                        color.b = (unsigned char)(color.b * brightness);
                        
                        DrawPixel(stripe, y, color);
                    }
                }
            }
        }
    }
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raycaster con Sprites");
    SetTargetFPS(60);
    
    // Inicializar buffer de profundidad
    depthBuffer = new float[SCREEN_WIDTH];
    
    // Crear texturas de sprites
    spriteTextures[0] = CreateCubeTexture(SKYBLUE);   // Cubo azul
    spriteTextures[1] = CreateCubeTexture(LIME);      // Cubo verde
    spriteTextures[2] = CreateCubeTexture(ORANGE);    // Cubo naranja
    
    // Inicializar sprites
    sprites.push_back({BLOCK_SIZE * 2.5f, BLOCK_SIZE * 2.5f, 0, true, 0}); // Cubo azul
    sprites.push_back({BLOCK_SIZE * 5.5f, BLOCK_SIZE * 5.5f, 1, true, 0}); // Cubo verde
    sprites.push_back({BLOCK_SIZE * 1.5f, BLOCK_SIZE * 6.5f, 2, true, 0}); // Cubo naranja
    sprites.push_back({BLOCK_SIZE * 6.5f, BLOCK_SIZE * 2.5f, 0, true, 0}); // Otro cubo azul
    
    Player player = {BLOCK_SIZE * 4, BLOCK_SIZE * 4, 0.0f, false};
    
    while (!WindowShouldClose()) {
        if (!player.hasWon) {
            float moveSpeed = 3.0f;
            if (IsKeyDown(KEY_W)) {
                float newX = player.x + cosf(player.angle) * moveSpeed;
                float newY = player.y + sinf(player.angle) * moveSpeed;
                int mapX = (int)(newX / BLOCK_SIZE);
                int mapY = (int)(newY / BLOCK_SIZE);
                if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                    if (worldMap[mapY][mapX] == 4) {
                        worldMap[mapY][mapX] = 0;
                        player.hasWon = true;
                    }
                    if (worldMap[mapY][mapX] == 0) {
                        player.x = newX;
                        player.y = newY;
                    }
                }
            }
            if (IsKeyDown(KEY_S)) {
                float newX = player.x - cosf(player.angle) * moveSpeed;
                float newY = player.y - sinf(player.angle) * moveSpeed;
                int mapX = (int)(newX / BLOCK_SIZE);
                int mapY = (int)(newY / BLOCK_SIZE);
                if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                    if (worldMap[mapY][mapX] == 4) {
                        worldMap[mapY][mapX] = 0;
                        player.hasWon = true;
                    }
                    if (worldMap[mapY][mapX] == 0) {
                        player.x = newX;
                        player.y = newY;
                    }
                }
            }
            if (IsKeyDown(KEY_A)) player.angle -= 0.05f;
            if (IsKeyDown(KEY_D)) player.angle += 0.05f;
        }
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Renderizar vista 3D y llenar buffer de profundidad
        for (int x = 0; x < NUM_RAYS; x++) {
            float rayAngle = player.angle - FOV/2 + (FOV * x / NUM_RAYS);
            
            Intersect hit = CastRay(player.x, player.y, rayAngle, BLOCK_SIZE, false);
            float correctedDistance = hit.distance * cosf(rayAngle - player.angle);
            
            // Guardar distancia en buffer de profundidad
            depthBuffer[x] = correctedDistance;
            
            float wallHeight = (SCREEN_HEIGHT / correctedDistance) * BLOCK_SIZE;
            
            int wallTop = (SCREEN_HEIGHT - wallHeight) / 2;
            int wallBottom = wallTop + wallHeight;
            
            Color wallColor;
            switch (hit.impact) {
                case '1': wallColor = RED; break;
                case '2': wallColor = BLUE; break;
                case '3': wallColor = GREEN; break;
                case '4': wallColor = PURPLE; break;
                default: wallColor = WHITE; break;
            }
            
            int brightness = (int)(255 / (1 + correctedDistance * 0.01f));
            wallColor.r = (wallColor.r * brightness) / 255;
            wallColor.g = (wallColor.g * brightness) / 255;
            wallColor.b = (wallColor.b * brightness) / 255;
            
            DrawLine(x, wallTop, x, wallBottom, wallColor);
        }
        
        // Calcular distancias de sprites y ordenar por profundidad
        for (auto& sprite : sprites) {
            if (sprite.active) {
                float dx = sprite.x - player.x;
                float dy = sprite.y - player.y;
                sprite.distance = dx * dx + dy * dy; // Distancia al cuadrado es suficiente para ordenar
            }
        }
        
        // Ordenar sprites por distancia (más lejanos primero)
        std::sort(sprites.begin(), sprites.end(), [](const Sprite& a, const Sprite& b) {
            return a.distance > b.distance;
        });
        
        // Dibujar sprites
        for (auto& sprite : sprites) {
            if (sprite.active) {
                DrawSprite(player, sprite, spriteTextures[sprite.type]);
            }
        }
        
        // Dibujar mini-mapa
        int mapScale = 60;
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                Color color;
                switch (worldMap[y][x]) {
                    case 0: color = BLACK; break;
                    case 1: color = WHITE; break;
                    case 2: color = BLUE; break;
                    case 3: color = GREEN; break;
                    case 4: color = PURPLE; break;
                    default: color = GRAY; break;
                }
                DrawRectangle(x * mapScale/MAP_WIDTH, y * mapScale/MAP_HEIGHT, 
                             mapScale/MAP_WIDTH, mapScale/MAP_HEIGHT, color);
            }
        }
        
        // Dibujar sprites en mini-mapa
        for (const auto& sprite : sprites) {
            if (sprite.active) {
                Color spriteMapColor;
                switch (sprite.type) {
                    case 0: spriteMapColor = SKYBLUE; break;
                    case 1: spriteMapColor = LIME; break;
                    case 2: spriteMapColor = ORANGE; break;
                }
                int spriteMapX = (sprite.x / BLOCK_SIZE) * mapScale/MAP_WIDTH;
                int spriteMapY = (sprite.y / BLOCK_SIZE) * mapScale/MAP_HEIGHT;
                DrawCircle(spriteMapX, spriteMapY, 2, spriteMapColor);
            }
        }
        
        // Dibujar jugador en mini-mapa
        int playerMapX = (player.x / BLOCK_SIZE) * mapScale/MAP_WIDTH;
        int playerMapY = (player.y / BLOCK_SIZE) * mapScale/MAP_HEIGHT;
        DrawCircle(playerMapX, playerMapY, 3, RED);
        
        int dirX = playerMapX + cosf(player.angle) * 10;
        int dirY = playerMapY + sinf(player.angle) * 10;
        DrawLine(playerMapX, playerMapY, dirX, dirY, YELLOW);
        
        // Instrucciones y estado del juego
        if (!player.hasWon) {
            DrawText("WASD: Mover/Girar", 10, SCREEN_HEIGHT - 30, 20, WHITE);
            DrawText("Encuentra el cubo morado!", 10, SCREEN_HEIGHT - 60, 20, YELLOW);
            DrawText("Sprites: Cubos decorativos", 10, SCREEN_HEIGHT - 90, 20, LIGHTGRAY);
        } else {
            // Pantalla de victoria (mismo código que antes)
            ClearBackground(DARKBLUE);
            
            const char* victoryText = "¡VICTORIA!";
            int titleFontSize = 60;
            int titleWidth = MeasureText(victoryText, titleFontSize);
            DrawText(victoryText, (SCREEN_WIDTH - titleWidth) / 2, 80, titleFontSize, GOLD);
            
            DrawText(victoryText, (SCREEN_WIDTH - titleWidth) / 2 + 3, 83, titleFontSize, DARKGRAY);
                    
            const char* congratsText = "¡Felicidades! Has completado el desafio";
            int congratsSize = 24;
            int congratsWidth = MeasureText(congratsText, congratsSize);
            DrawText(congratsText, (SCREEN_WIDTH - congratsWidth) / 2, 420, congratsSize, WHITE);
            
            const char* missionText = "Si lees esto, sos un tonto (esto es un placeholder)";
            int missionSize = 18;
            int missionWidth = MeasureText(missionText, missionSize);
            DrawText(missionText, (SCREEN_WIDTH - missionWidth) / 2, 450, missionSize, LIGHTGRAY);
            
            static float blinkTimer = 0.0f;
            blinkTimer += GetFrameTime();
            
            if ((int)(blinkTimer * 2) % 2 == 0) {
                const char* exitText = "Presiona ENTER para salir";
                int exitSize = 22;
                int exitWidth = MeasureText(exitText, exitSize);
                
                DrawRectangle((SCREEN_WIDTH - exitWidth) / 2 - 20, 500, exitWidth + 40, 40, DARKGREEN);
                DrawRectangleLines((SCREEN_WIDTH - exitWidth) / 2 - 20, 500, exitWidth + 40, 40, GREEN);
                
                DrawText(exitText, (SCREEN_WIDTH - exitWidth) / 2, 510, exitSize, YELLOW);
            }
            
            if (IsKeyPressed(KEY_ENTER)) {
                CloseWindow();
                return 0;
            }
        }
        
        EndDrawing();
    }
    
    // Limpiar memoria
    delete[] depthBuffer;
    for (int i = 0; i < 3; i++) {
        delete[] spriteTextures[i];
    }
    
    CloseWindow();
    return 0;
}