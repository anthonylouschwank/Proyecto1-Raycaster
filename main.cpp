#include "raylib.h"
#include <cmath>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;
const float FOV = 60.0f * DEG2RAD; // Campo de vision en radianes
const float BLOCK_SIZE = 64.0f; // Tamaño de cada bloque del mapa
const int NUM_RAYS = SCREEN_WIDTH;

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
        
        if (worldMap[j][i] != 0) {
            // Si es el cubo morado (4), no es solido para los rayos pero si visible
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

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raycaster Simple");
    SetTargetFPS(60);
    
    Player player = {BLOCK_SIZE * 4, BLOCK_SIZE * 4, 0.0f, false}; // Posicion inicial con estado de victoria
    
    while (!WindowShouldClose()) {
        // Controles del jugador (solo si no ha ganado)
        if (!player.hasWon) {
            float moveSpeed = 3.0f;
            if (IsKeyDown(KEY_W)) {
                float newX = player.x + cosf(player.angle) * moveSpeed;
                float newY = player.y + sinf(player.angle) * moveSpeed;
                int mapX = (int)(newX / BLOCK_SIZE);
                int mapY = (int)(newY / BLOCK_SIZE);
                if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                    // Verificar si recogio el cubo morado
                    if (worldMap[mapY][mapX] == 4) {
                        worldMap[mapY][mapX] = 0; // Remover el cubo del mapa
                        player.hasWon = true;     // Activar estado de victoria
                    }
                    // Solo moverse si no es una pared solida (1, 2, 3)
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
                    // Verificar si recogio el cubo morado
                    if (worldMap[mapY][mapX] == 4) {
                        worldMap[mapY][mapX] = 0; // Remover el cubo del mapa
                        player.hasWon = true;     // Activar estado de victoria
                    }
                    // Solo moverse si no es una pared solida (1, 2, 3)
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
        
        // Renderizar vista 3D
        for (int x = 0; x < NUM_RAYS; x++) {
            // Calcular angulo del rayo
            float rayAngle = player.angle - FOV/2 + (FOV * x / NUM_RAYS);
            
            // Lanzar rayo con el nuevo algoritmo
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
                case '4': wallColor = PURPLE; break;   // Cubo morado (objetivo)
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
                    case 4: color = PURPLE; break;   // Cubo morado (objetivo)
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
        
        // Instrucciones y estado del juego
        if (!player.hasWon) {
            DrawText("WASD: Mover/Girar", 10, SCREEN_HEIGHT - 30, 20, WHITE);
            DrawText("Encuentra el cubo morado!", 10, SCREEN_HEIGHT - 60, 20, YELLOW);
        } else {
            // Pantalla de victoria formal
            ClearBackground(DARKBLUE); // Fondo azul oscuro elegante
            
            const char* victoryText = "¡VICTORIA!";
            int titleFontSize = 60;
            int titleWidth = MeasureText(victoryText, titleFontSize);
            DrawText(victoryText, (SCREEN_WIDTH - titleWidth) / 2, 80, titleFontSize, GOLD);
            
            // Sombra del titulo para efecto 3D
            DrawText(victoryText, (SCREEN_WIDTH - titleWidth) / 2 + 3, 83, titleFontSize, DARKGRAY);
            
            // Rectangulo decorativo para el area del sprite
            int spriteBoxX = SCREEN_WIDTH / 2 - 100;
            int spriteBoxY = 180;
            int spriteBoxWidth = 200;
            int spriteBoxHeight = 200;
            
            DrawRectangle(spriteBoxX - 5, spriteBoxY - 5, spriteBoxWidth + 10, spriteBoxHeight + 10, GOLD);
            DrawRectangle(spriteBoxX, spriteBoxY, spriteBoxWidth, spriteBoxHeight, WHITE);
            
            // Texto placeholder para el sprite
            const char* spriteText = "SPRITE";
            const char* spriteText2 = "ALGO";
            const char* spriteText3 = "AQUi";
            int spriteTextSize = 20;
            int spriteWidth1 = MeasureText(spriteText, spriteTextSize);
            int spriteWidth2 = MeasureText(spriteText2, spriteTextSize);
            int spriteWidth3 = MeasureText(spriteText3, spriteTextSize);
            
            DrawText(spriteText, spriteBoxX + (spriteBoxWidth - spriteWidth1) / 2, spriteBoxY + 70, spriteTextSize, GRAY);
            DrawText(spriteText2, spriteBoxX + (spriteBoxWidth - spriteWidth2) / 2, spriteBoxY + 100, spriteTextSize, RED);
            DrawText(spriteText3, spriteBoxX + (spriteBoxWidth - spriteWidth3) / 2, spriteBoxY + 130, spriteTextSize, GRAY);
            
            // Mensaje de felicitacion
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
            
            if ((int)(blinkTimer * 2) % 2 == 0) { // Parpadea cada 0.5 segundos
                const char* exitText = "Presiona ENTER para salir";
                int exitSize = 22;
                int exitWidth = MeasureText(exitText, exitSize);
                
                // Fondo del boton
                DrawRectangle((SCREEN_WIDTH - exitWidth) / 2 - 20, 500, exitWidth + 40, 40, DARKGREEN);
                DrawRectangleLines((SCREEN_WIDTH - exitWidth) / 2 - 20, 500, exitWidth + 40, 40, GREEN);
                
                DrawText(exitText, (SCREEN_WIDTH - exitWidth) / 2, 510, exitSize, YELLOW);
            }
            
            // Verificar si se presiona ENTER para cerrar el juego
            if (IsKeyPressed(KEY_ENTER)) {
                CloseWindow();
                return 0;
            }
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}