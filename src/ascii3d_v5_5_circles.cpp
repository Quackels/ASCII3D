#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>

#pragma comment(lib, "user32.lib")

constexpr int SCREEN_WIDTH  = 140;
constexpr int SCREEN_HEIGHT = 45;

constexpr int MAP_WIDTH  = 24;
constexpr int MAP_HEIGHT = 24;

constexpr int MINIMAP_SIZE = 13;
constexpr int MINIMAP_X = 2;
constexpr int MINIMAP_Y = 2;

constexpr float PI = 3.1415926535f;
constexpr float TERMINAL_ASPECT = 0.55f;

void PutChar(std::vector<wchar_t>& frame, int x, int y, wchar_t c)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
        return;

    frame[y * SCREEN_WIDTH + x] = c;
}

void EnableVT()
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (GetConsoleMode(out, &mode))
    {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(out, mode);
    }
}

void WriteVT(const std::wstring& text)
{
    DWORD written = 0;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    WriteConsoleW(
        out,
        text.c_str(),
        static_cast<DWORD>(text.size()),
        &written,
        nullptr
    );
}


std::wstring BuildCircularMap()
{
    std::wstring map(MAP_WIDTH * MAP_HEIGHT, L'.');

    // Outer boundary
    for (int x = 0; x < MAP_WIDTH; x++)
    {
        map[x] = L'#';
        map[(MAP_HEIGHT - 1) * MAP_WIDTH + x] = L'#';
    }

    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        map[y * MAP_WIDTH] = L'#';
        map[y * MAP_WIDTH + (MAP_WIDTH - 1)] = L'#';
    }

    auto addSolidCircle = [&](float cx, float cy, float radius)
    {
        for (int y = 1; y < MAP_HEIGHT - 1; y++)
        {
            for (int x = 1; x < MAP_WIDTH - 1; x++)
            {
                float dx = (x + 0.5f) - cx;
                float dy = (y + 0.5f) - cy;

                if (dx * dx + dy * dy <= radius * radius)
                    map[y * MAP_WIDTH + x] = L'#';
            }
        }
    };

    auto addRing = [&](float cx, float cy, float radius, float thickness)
    {
        for (int y = 1; y < MAP_HEIGHT - 1; y++)
        {
            for (int x = 1; x < MAP_WIDTH - 1; x++)
            {
                float dx = (x + 0.5f) - cx;
                float dy = (y + 0.5f) - cy;
                float d = sqrtf(dx * dx + dy * dy);

                if (fabsf(d - radius) <= thickness)
                    map[y * MAP_WIDTH + x] = L'#';
            }
        }
    };

    // Big solid circular obstacle
    addSolidCircle(7.5f, 7.5f, 3.6f);

    // Hollow circular room / ring
    addRing(17.0f, 14.0f, 4.2f, 0.55f);

    // Cut a doorway into the circular ring
    map[14 * MAP_WIDTH + 12] = L'.';
    map[13 * MAP_WIDTH + 12] = L'.';

    // Small round pillar
    addSolidCircle(6.5f, 17.5f, 1.8f);

    return map;
}

int main()
{
    EnableVT();

    // Enter alternate screen, clear it, move cursor home, hide cursor.
    WriteVT(L"\x1b[?1049h\x1b[2J\x1b[H\x1b[?25l");

    std::vector<wchar_t> frame(SCREEN_WIDTH * SCREEN_HEIGHT, L' ');

    std::wstring map = BuildCircularMap();

    float playerX = 18.0f;
    float playerY = 20.0f;
    float playerAngle = 0.0f;

    constexpr float MOVE_SPEED = 4.0f;
    constexpr float TURN_SPEED = 2.0f;
    constexpr float FOV = 60.0f * PI / 180.0f;
    constexpr float MAX_DEPTH = 32.0f;

    auto lastFrame = std::chrono::steady_clock::now();

    bool running = true;

    while (running)
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - lastFrame;
        lastFrame = now;

        float dt = std::clamp(elapsed.count(), 0.0001f, 0.05f);

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            running = false;

        if (GetAsyncKeyState(VK_LEFT) & 0x8000)
            playerAngle -= TURN_SPEED * dt;

        if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
            playerAngle += TURN_SPEED * dt;

        if (playerAngle > PI * 2.0f)
            playerAngle -= PI * 2.0f;

        if (playerAngle < 0.0f)
            playerAngle += PI * 2.0f;

        float forwardX = cosf(playerAngle);
        float forwardY = sinf(playerAngle);

        float rightX = -forwardY;
        float rightY = forwardX;

        float velocityX = 0.0f;
        float velocityY = 0.0f;

        if (GetAsyncKeyState('W') & 0x8000)
        {
            velocityX += forwardX;
            velocityY += forwardY;
        }

        if (GetAsyncKeyState('S') & 0x8000)
        {
            velocityX -= forwardX;
            velocityY -= forwardY;
        }

        if (GetAsyncKeyState('A') & 0x8000)
        {
            velocityX -= rightX;
            velocityY -= rightY;
        }

        if (GetAsyncKeyState('D') & 0x8000)
        {
            velocityX += rightX;
            velocityY += rightY;
        }

        float movementLength =
            sqrtf(velocityX * velocityX + velocityY * velocityY);

        if (movementLength > 0.0f)
        {
            velocityX /= movementLength;
            velocityY /= movementLength;

            velocityX *= MOVE_SPEED * dt;
            velocityY *= MOVE_SPEED * dt;
        }

        float newX = playerX + velocityX;
        float newY = playerY + velocityY;

        int testX = static_cast<int>(newX);
        int testY = static_cast<int>(playerY);

        if (
            testX >= 0 &&
            testX < MAP_WIDTH &&
            testY >= 0 &&
            testY < MAP_HEIGHT &&
            map[testY * MAP_WIDTH + testX] != L'#'
        )
        {
            playerX = newX;
        }

        testX = static_cast<int>(playerX);
        testY = static_cast<int>(newY);

        if (
            testX >= 0 &&
            testX < MAP_WIDTH &&
            testY >= 0 &&
            testY < MAP_HEIGHT &&
            map[testY * MAP_WIDTH + testX] != L'#'
        )
        {
            playerY = newY;
        }

        std::fill(frame.begin(), frame.end(), L' ');

        float dirX = cosf(playerAngle);
        float dirY = sinf(playerAngle);

        float planeLength = tanf(FOV / 2.0f);

        float planeX = -dirY * planeLength;
        float planeY =  dirX * planeLength;

        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            float cameraX =
                2.0f * x / static_cast<float>(SCREEN_WIDTH) - 1.0f;

            float rayDirX = dirX + planeX * cameraX;
            float rayDirY = dirY + planeY * cameraX;

            int cellX = static_cast<int>(playerX);
            int cellY = static_cast<int>(playerY);

            float deltaDistX =
                (rayDirX == 0.0f)
                ? std::numeric_limits<float>::infinity()
                : fabsf(1.0f / rayDirX);

            float deltaDistY =
                (rayDirY == 0.0f)
                ? std::numeric_limits<float>::infinity()
                : fabsf(1.0f / rayDirY);

            int stepX;
            int stepY;

            float sideDistX;
            float sideDistY;

            if (rayDirX < 0.0f)
            {
                stepX = -1;
                sideDistX = (playerX - cellX) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (cellX + 1.0f - playerX) * deltaDistX;
            }

            if (rayDirY < 0.0f)
            {
                stepY = -1;
                sideDistY = (playerY - cellY) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (cellY + 1.0f - playerY) * deltaDistY;
            }

            bool hit = false;
            int side = 0;

            while (!hit)
            {
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX;
                    cellX += stepX;
                    side = 0;
                }
                else
                {
                    sideDistY += deltaDistY;
                    cellY += stepY;
                    side = 1;
                }

                if (
                    cellX < 0 ||
                    cellX >= MAP_WIDTH ||
                    cellY < 0 ||
                    cellY >= MAP_HEIGHT
                )
                {
                    break;
                }

                if (map[cellY * MAP_WIDTH + cellX] == L'#')
                    hit = true;
            }

            float wallDistance =
                (side == 0)
                ? sideDistX - deltaDistX
                : sideDistY - deltaDistY;

            wallDistance =
                std::clamp(wallDistance, 0.05f, MAX_DEPTH);

            float wallHit;

            if (side == 0)
                wallHit = playerY + wallDistance * rayDirY;
            else
                wallHit = playerX + wallDistance * rayDirX;

            wallHit -= floorf(wallHit);

            int wallHeight =
                static_cast<int>(
                    (SCREEN_HEIGHT / wallDistance) *
                    TERMINAL_ASPECT
                );

            wallHeight =
                std::clamp(wallHeight, 1, SCREEN_HEIGHT);

            int wallTop =
                SCREEN_HEIGHT / 2 -
                wallHeight / 2;

            int wallBottom =
                SCREEN_HEIGHT / 2 +
                wallHeight / 2;

            wallTop =
                std::max(wallTop, 1);

            wallBottom =
                std::min(wallBottom, SCREEN_HEIGHT - 1);

            for (int y = 1; y < SCREEN_HEIGHT; y++)
            {
                if (y < wallTop)
                {
                    PutChar(frame, x, y, L' ');
                }
                else if (y <= wallBottom)
                {
                    float verticalPosition =
                        static_cast<float>(y - wallTop) /
                        std::max(1, wallBottom - wallTop);

                    int textureX =
                        static_cast<int>(wallHit * 12.0f);

                    int textureY =
                        static_cast<int>(verticalPosition * 12.0f);

                    bool horizontalMortar =
                        (textureY % 4) == 0;

                    bool offsetRow =
                        ((textureY / 4) % 2) == 1;

                    int brickX =
                        textureX + (offsetRow ? 2 : 0);

                    bool verticalMortar =
                        (brickX % 6) == 0;

                    wchar_t wallChar;

                    if (horizontalMortar || verticalMortar)
                    {
                        wallChar = L'.';
                    }
                    else
                    {
                        if (wallDistance < 3.0f)
                            wallChar = L'█';
                        else if (wallDistance < 6.0f)
                            wallChar = L'▓';
                        else if (wallDistance < 10.0f)
                            wallChar = L'▒';
                        else if (wallDistance < 18.0f)
                            wallChar = L'░';
                        else
                            wallChar = L':';

                        if (side == 1)
                        {
                            if (wallChar == L'█')
                                wallChar = L'▓';
                            else if (wallChar == L'▓')
                                wallChar = L'▒';
                            else if (wallChar == L'▒')
                                wallChar = L'░';
                            else if (wallChar == L'░')
                                wallChar = L':';
                        }
                    }

                    PutChar(frame, x, y, wallChar);
                }
                else
                {
                    float floorDepth =
                        (y - SCREEN_HEIGHT / 2.0f) /
                        (SCREEN_HEIGHT / 2.0f);

                    wchar_t floorChar;

                    if (floorDepth > 0.82f)
                        floorChar = L'#';
                    else if (floorDepth > 0.64f)
                        floorChar = L'x';
                    else if (floorDepth > 0.46f)
                        floorChar = L'.';
                    else if (floorDepth > 0.28f)
                        floorChar = L'-';
                    else
                        floorChar = L' ';

                    PutChar(frame, x, y, floorChar);
                }
            }
        }

        // ====================================================
        // MINIMAP
        // ====================================================

        constexpr int HALF_MAP = MINIMAP_SIZE / 2;

        int playerCellX = static_cast<int>(playerX);
        int playerCellY = static_cast<int>(playerY);

        for (int miniY = 0; miniY < MINIMAP_SIZE; miniY++)
        {
            for (int miniX = 0; miniX < MINIMAP_SIZE; miniX++)
            {
                int worldX =
                    playerCellX +
                    miniX -
                    HALF_MAP;

                int worldY =
                    playerCellY +
                    miniY -
                    HALF_MAP;

                wchar_t symbol = L'#';

                if (
                    worldX >= 0 &&
                    worldX < MAP_WIDTH &&
                    worldY >= 0 &&
                    worldY < MAP_HEIGHT
                )
                {
                    symbol =
                        (map[worldY * MAP_WIDTH + worldX] == L'#')
                        ? L'#'
                        : L'.';
                }

                PutChar(
                    frame,
                    MINIMAP_X + miniX,
                    MINIMAP_Y + miniY,
                    symbol
                );
            }
        }

        int miniPlayerX =
            MINIMAP_X + HALF_MAP;

        int miniPlayerY =
            MINIMAP_Y + HALF_MAP;

        PutChar(
            frame,
            miniPlayerX,
            miniPlayerY,
            L'P'
        );

        int facingX =
            static_cast<int>(roundf(cosf(playerAngle)));

        int facingY =
            static_cast<int>(roundf(sinf(playerAngle)));

        PutChar(
            frame,
            miniPlayerX + facingX,
            miniPlayerY + facingY,
            L'*'
        );

        PutChar(
            frame,
            SCREEN_WIDTH / 2,
            SCREEN_HEIGHT / 2,
            L'+'
        );

        // ====================================================
        // HUD
        // ====================================================

        float fps = 1.0f / dt;

        wchar_t hud[SCREEN_WIDTH];

        swprintf_s(
            hud,
            SCREEN_WIDTH,
            L"ASCII3D V5.5 CIRCLES | FPS %.0f | X %.1f Y %.1f | VT Alternate Screen | WASD Move | Arrows Turn | ESC",
            fps,
            playerX,
            playerY
        );

        for (
            int i = 0;
            hud[i] != L'\0' &&
            i < SCREEN_WIDTH;
            i++
        )
        {
            PutChar(frame, i, 0, hud[i]);
        }

        // ====================================================
        // BUILD ONE TERMINAL FRAME
        // ====================================================

        std::wstring output;
        output.reserve(
            SCREEN_WIDTH * SCREEN_HEIGHT +
            SCREEN_HEIGHT * 2 +
            8
        );

        // Move cursor to 1,1.
        output += L"\x1b[H";

        for (int y = 0; y < SCREEN_HEIGHT; y++)
        {
            for (int x = 0; x < SCREEN_WIDTH; x++)
            {
                output += frame[
                    y * SCREEN_WIDTH +
                    x
                ];
            }

            // IMPORTANT:
            // no newline after final row.
            if (y != SCREEN_HEIGHT - 1)
                output += L"\r\n";
        }

        WriteVT(output);

        Sleep(8);
    }

    // Restore cursor and leave alternate screen.
    WriteVT(L"\x1b[?25h\x1b[?1049l");

    return 0;
}
