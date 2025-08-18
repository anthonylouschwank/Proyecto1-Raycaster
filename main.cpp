#include "raylib.h"
#include <cmath>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;
const float FOV = 60.0f * DEG2RAD; // Campo de vision en radianes
const float BLOCK_SIZE = 64.0f; // Tamaño de cada bloque del mapa
const int NUM_RAYS = SCREEN_WIDTH;

// Mapa con diferentes tipos de paredes
int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,2,0,0,3,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,2,0,0,3,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1}
};

struct Player {
    float x, y;        
    float angle;       
};

// Funcion para verificar si una posicion esta dentro del mapa
bool IsWall(float x, float y) {
    int mapX = (int)x;
    int mapY = (int)y;
    
    if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
        return true; // Fuera del mapa = pared
    }
    
    return worldMap[mapY][mapX] == 1;
}

struct Intersect {
    float distance;
    char impact;
};

// Funcion principal de raycasting 
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
            return {d, '1'}; // Fuera del mapa = pared
        }
        
        // Verificar si hay pared
        if (worldMap[j][i] != 0) {
            return {d, (char)('0' + worldMap[j][i])};
        }
        
        if (drawLine) {
            DrawPixel((int)x * 60 / MAP_WIDTH, (int)y * 60 / MAP_HEIGHT, YELLOW);
        }
        
        d += 5.0f; 
    }
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raycaster Simple");
    SetTargetFPS(60);
    
    Player player = {BLOCK_SIZE * 4, BLOCK_SIZE * 4, 0.0f}; // Posicion inicial 
    
    while (!WindowShouldClose()) {
        // Controles del jugador
        float moveSpeed = 3.0f;
        if (IsKeyDown(KEY_W)) {
            float newX = player.x + cosf(player.angle) * moveSpeed;
            float newY = player.y + sinf(player.angle) * moveSpeed;
            int mapX = (int)(newX / BLOCK_SIZE);
            int mapY = (int)(newY / BLOCK_SIZE);
            if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT && worldMap[mapY][mapX] == 0) {
                player.x = newX;
                player.y = newY;
            }
        }
        if (IsKeyDown(KEY_S)) {
            float newX = player.x - cosf(player.angle) * moveSpeed;
            float newY = player.y - sinf(player.angle) * moveSpeed;
            int mapX = (int)(newX / BLOCK_SIZE);
            int mapY = (int)(newY / BLOCK_SIZE);
            if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT && worldMap[mapY][mapX] == 0) {
                player.x = newX;
                player.y = newY;
            }
        }
        if (IsKeyDown(KEY_A)) player.angle -= 0.05f;
        if (IsKeyDown(KEY_D)) player.angle += 0.05f;
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Renderizar vista 3D
        for (int x = 0; x < NUM_RAYS; x++) {
            // Calcular angulo del rayo
            float rayAngle = player.angle - FOV/2 + (FOV * x / NUM_RAYS);
            
            // Lanzar rayo
            Intersect hit = CastRay(player.x, player.y, rayAngle, BLOCK_SIZE, false);
            float correctedDistance = hit.distance * cosf(rayAngle - player.angle);
            
            // Calcular altura de la pared en pantalla
            float wallHeight = (SCREEN_HEIGHT / correctedDistance) * BLOCK_SIZE;
            
            // Dibujar columna vertical
            int wallTop = (SCREEN_HEIGHT - wallHeight) / 2;
            int wallBottom = wallTop + wallHeight;
            
            // Color basado en el tipo de pared
            Color wallColor;
            switch (hit.impact) {
                case '1': wallColor = RED; break;      // Paredes normales
                case '2': wallColor = BLUE; break;     // Paredes especiales
                case '3': wallColor = GREEN; break;    // Otras paredes
                default: wallColor = WHITE; break;
            }
            
            // Aplicar sombreado por distancia
            int brightness = (int)(255 / (1 + correctedDistance * 0.01f));
            wallColor.r = (wallColor.r * brightness) / 255;
            wallColor.g = (wallColor.g * brightness) / 255;
            wallColor.b = (wallColor.b * brightness) / 255;
            
            DrawLine(x, wallTop, x, wallBottom, wallColor);
        }
        
        // Dibujar mini-mapa
        int mapScale = 60;
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                Color color;
                switch (worldMap[y][x]) {
                    case 0: color = BLACK; break;    // Espacio vacio
                    case 1: color = WHITE; break;    // Pared normal
                    case 2: color = BLUE; break;     // Pared especial
                    case 3: color = GREEN; break;    // Otra pared
                    default: color = GRAY; break;
                }
                DrawRectangle(x * mapScale/MAP_WIDTH, y * mapScale/MAP_HEIGHT, 
                             mapScale/MAP_WIDTH, mapScale/MAP_HEIGHT, color);
            }
        }
        
        // Dibujar jugador en mini-mapa
        int playerMapX = (player.x / BLOCK_SIZE) * mapScale/MAP_WIDTH;
        int playerMapY = (player.y / BLOCK_SIZE) * mapScale/MAP_HEIGHT;
        DrawCircle(playerMapX, playerMapY, 3, RED);
        
        // Dibujar direccion del jugador
        int dirX = playerMapX + cosf(player.angle) * 10;
        int dirY = playerMapY + sinf(player.angle) * 10;
        DrawLine(playerMapX, playerMapY, dirX, dirY, YELLOW);
        
        // Instrucciones
        DrawText("WASD: Mover/Girar", 10, SCREEN_HEIGHT - 30, 20, WHITE);
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}