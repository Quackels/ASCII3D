#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <limits>

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

struct Sprite
{
    float x;
    float y;
    bool alive;
    wchar_t mapIcon;
    int health;
    float attackCooldown;
};

std::vector<int> gColors(SCREEN_WIDTH * SCREEN_HEIGHT, 37);

void PutChar(
    std::vector<wchar_t>& frame,
    int x,
    int y,
    wchar_t c,
    int ansiColor = 37
)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
        return;

    int index = y * SCREEN_WIDTH + x;

    frame[index] = c;
    gColors[index] = ansiColor;
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

    addSolidCircle(7.5f, 7.5f, 3.6f);
    addRing(17.0f, 14.0f, 4.2f, 0.55f);

    map[14 * MAP_WIDTH + 12] = L'.';
    map[13 * MAP_WIDTH + 12] = L'.';

    addSolidCircle(6.5f, 17.5f, 1.8f);

    return map;
}

void DrawWeapon(std::vector<wchar_t>& frame, bool muzzleFlash)
{
    // Simple first-person pistol, centered at bottom.
    static const std::vector<std::wstring> gun =
    {
        L"            ______            ",
        L"           / ____ \\           ",
        L"          / /____\\ \\          ",
        L"         |  _____  |          ",
        L"         | |     | |          ",
        L"         | |_____| |          ",
        L"         |_________|          ",
        L"            ||||             ",
        L"            ||||             "
    };

    int gunWidth = static_cast<int>(gun[0].size());
    int startX = SCREEN_WIDTH / 2 - gunWidth / 2;
    int startY = SCREEN_HEIGHT - static_cast<int>(gun.size());

    for (int gy = 0; gy < static_cast<int>(gun.size()); gy++)
    {
        for (int gx = 0; gx < static_cast<int>(gun[gy].size()); gx++)
        {
            wchar_t c = gun[gy][gx];

            if (c != L' ')
                PutChar(frame, startX + gx, startY + gy, c, 93);
        }
    }

    if (muzzleFlash)
    {
        int cx = SCREEN_WIDTH / 2;

        PutChar(frame, cx,     startY - 4, L'*', 91);
        PutChar(frame, cx - 1, startY - 3, L'\\', 91);
        PutChar(frame, cx + 1, startY - 3, L'/', 91);
        PutChar(frame, cx - 2, startY - 2, L'-', 91);
        PutChar(frame, cx,     startY - 2, L'*', 91);
        PutChar(frame, cx + 2, startY - 2, L'-', 91);
        PutChar(frame, cx - 1, startY - 1, L'/', 91);
        PutChar(frame, cx + 1, startY - 1, L'\\', 91);
    }
}

void DrawEnemySprite(
    std::vector<wchar_t>& frame,
    const Sprite& sprite,
    float playerX,
    float playerY,
    float dirX,
    float dirY,
    float planeX,
    float planeY,
    const std::vector<float>& zBuffer
)
{
    if (!sprite.alive)
        return;

    float relX = sprite.x - playerX;
    float relY = sprite.y - playerY;

    float determinant = planeX * dirY - dirX * planeY;

    if (fabsf(determinant) < 0.0001f)
        return;

    float invDet = 1.0f / determinant;

    float transformX =
        invDet * (dirY * relX - dirX * relY);

    float transformY =
        invDet * (-planeY * relX + planeX * relY);

    // Behind camera.
    if (transformY <= 0.1f)
        return;

    int spriteScreenX =
        static_cast<int>(
            (SCREEN_WIDTH / 2.0f) *
            (1.0f + transformX / transformY)
        );

    int spriteHeight =
        abs(static_cast<int>(
            (SCREEN_HEIGHT / transformY) *
            0.78f
        ));

    spriteHeight = std::clamp(spriteHeight, 3, SCREEN_HEIGHT - 4);

    // Terminal cells are tall, so make the sprite wider.
    int spriteWidth =
        std::max(3, static_cast<int>(spriteHeight * 1.85f));

    int drawStartY =
        SCREEN_HEIGHT / 2 - spriteHeight / 2;

    int drawEndY =
        drawStartY + spriteHeight;

    int drawStartX =
        spriteScreenX - spriteWidth / 2;

    int drawEndX =
        spriteScreenX + spriteWidth / 2;

    drawStartY = std::max(drawStartY, 1);
    drawEndY = std::min(drawEndY, SCREEN_HEIGHT - 1);

    // 16 x 12 source sprite.
    static const std::vector<std::wstring> enemyTexture =
    {
        L"      /\\      ",
        L"   __/  \\__   ",
        L"  /  o  o  \\  ",
        L" /    /\\    \\ ",
        L"|    ____    |",
        L"|   /####\\   |",
        L"|   \\####/   |",
        L"|  /|####|\\  |",
        L" \\   ----   / ",
        L"  \\________/  ",
        L"    / || \\    ",
        L"   /__||__\\   "
    };

    const int texH = static_cast<int>(enemyTexture.size());
    const int texW = static_cast<int>(enemyTexture[0].size());

    for (int stripe = drawStartX; stripe <= drawEndX; stripe++)
    {
        if (stripe < 0 || stripe >= SCREEN_WIDTH)
            continue;

        // Wall in front of sprite? Don't draw this column.
        if (transformY >= zBuffer[stripe])
            continue;

        float u =
            static_cast<float>(stripe - drawStartX) /
            std::max(1, drawEndX - drawStartX);

        int texX =
            std::clamp(
                static_cast<int>(u * (texW - 1)),
                0,
                texW - 1
            );

        for (int y = drawStartY; y <= drawEndY; y++)
        {
            float v =
                static_cast<float>(y - drawStartY) /
                std::max(1, drawEndY - drawStartY);

            int texY =
                std::clamp(
                    static_cast<int>(v * (texH - 1)),
                    0,
                    texH - 1
                );

            wchar_t c = enemyTexture[texY][texX];

            // Spaces are transparent.
            if (c != L' ')
                PutChar(frame, stripe, y, c, 91);
        }
    }
}

int main()
{
    EnableVT();

    WriteVT(L"\x1b[?1049h\x1b[2J\x1b[H\x1b[?25l");

    std::vector<wchar_t> frame(SCREEN_WIDTH * SCREEN_HEIGHT, L' ');
    std::vector<float> zBuffer(SCREEN_WIDTH, 9999.0f);

    std::wstring map = BuildCircularMap();

    float playerX = 18.0f;
    float playerY = 20.0f;
    float playerAngle = 0.0f;

    constexpr float MOVE_SPEED = 4.0f;
    constexpr float TURN_SPEED = 2.0f;
    constexpr float FOV = 60.0f * PI / 180.0f;
    constexpr float MAX_DEPTH = 32.0f;

    std::vector<Sprite> sprites =
    {
        { 18.0f, 8.0f,  true, L'E', 2, 0.0f },
        { 11.0f, 19.0f, true, L'E', 2, 0.0f },
        { 20.0f, 17.0f, true, L'E', 2, 0.0f }
    };

    bool previousShootKey = false;
    float muzzleTimer = 0.0f;
    float recoilTimer = 0.0f;
    int kills = 0;
    int playerHealth = 100;
    int ammo = 12;
    int reserveAmmo = 48;
    bool previousReloadKey = false;
    float damageFlashTimer = 0.0f;

    auto lastFrame = std::chrono::steady_clock::now();

    bool running = true;

    while (running)
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - lastFrame;
        lastFrame = now;

        float dt = std::clamp(elapsed.count(), 0.0001f, 0.05f);

        if (muzzleTimer > 0.0f)
            muzzleTimer -= dt;

        if (recoilTimer > 0.0f)
            recoilTimer -= dt;

        if (damageFlashTimer > 0.0f)
            damageFlashTimer -= dt;

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
        std::fill(gColors.begin(), gColors.end(), 37);
        std::fill(zBuffer.begin(), zBuffer.end(), MAX_DEPTH);

        float dirX = cosf(playerAngle);
        float dirY = sinf(playerAngle);

        float planeLength = tanf(FOV / 2.0f);

        float planeX = -dirY * planeLength;
        float planeY =  dirX * planeLength;

        // ====================================================
        // RELOAD
        // ====================================================

        bool reloadKey =
            (GetAsyncKeyState('R') & 0x8000) != 0;

        if (reloadKey && !previousReloadKey && ammo < 12 && reserveAmmo > 0)
        {
            int needed = 12 - ammo;
            int loaded = std::min(needed, reserveAmmo);

            ammo += loaded;
            reserveAmmo -= loaded;
        }

        previousReloadKey = reloadKey;

        // ====================================================
        // SHOOTING
        // ====================================================

        bool shootKey =
            (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

        if (shootKey && !previousShootKey && ammo > 0)
        {
            ammo--;
            muzzleTimer = 0.10f;
            recoilTimer = 0.12f;

            int bestTarget = -1;
            float bestDistance = std::numeric_limits<float>::infinity();

            for (int i = 0; i < static_cast<int>(sprites.size()); i++)
            {
                if (!sprites[i].alive)
                    continue;

                float relX = sprites[i].x - playerX;
                float relY = sprites[i].y - playerY;

                float distance = sqrtf(relX * relX + relY * relY);
                float angleToTarget = atan2f(relY, relX);
                float delta = angleToTarget - playerAngle;

                while (delta > PI) delta -= PI * 2.0f;
                while (delta < -PI) delta += PI * 2.0f;

                float hitCone =
                    std::max(0.025f, 0.22f / std::max(distance, 1.0f));

                if (fabsf(delta) < hitCone && distance < bestDistance)
                {
                    bestDistance = distance;
                    bestTarget = i;
                }
            }

            if (bestTarget >= 0)
            {
                sprites[bestTarget].health--;

                if (sprites[bestTarget].health <= 0)
                {
                    sprites[bestTarget].alive = false;
                    kills++;
                }
            }
        }

        previousShootKey = shootKey;

        // ====================================================
        // ENEMY AI
        // ====================================================

        for (Sprite& enemy : sprites)
        {
            if (!enemy.alive)
                continue;

            if (enemy.attackCooldown > 0.0f)
                enemy.attackCooldown -= dt;

            float dx = playerX - enemy.x;
            float dy = playerY - enemy.y;
            float distance = sqrtf(dx * dx + dy * dy);

            if (distance > 0.001f)
            {
                dx /= distance;
                dy /= distance;
            }

            // Chase when within 12 map units.
            if (distance < 12.0f && distance > 0.85f)
            {
                constexpr float ENEMY_SPEED = 1.15f;

                float enemyNewX = enemy.x + dx * ENEMY_SPEED * dt;
                float enemyNewY = enemy.y + dy * ENEMY_SPEED * dt;

                int ex = static_cast<int>(enemyNewX);
                int ey = static_cast<int>(enemy.y);

                if (
                    ex >= 0 && ex < MAP_WIDTH &&
                    ey >= 0 && ey < MAP_HEIGHT &&
                    map[ey * MAP_WIDTH + ex] != L'#'
                )
                {
                    enemy.x = enemyNewX;
                }

                ex = static_cast<int>(enemy.x);
                ey = static_cast<int>(enemyNewY);

                if (
                    ex >= 0 && ex < MAP_WIDTH &&
                    ey >= 0 && ey < MAP_HEIGHT &&
                    map[ey * MAP_WIDTH + ex] != L'#'
                )
                {
                    enemy.y = enemyNewY;
                }
            }

            // Melee attack.
            if (distance <= 1.05f && enemy.attackCooldown <= 0.0f)
            {
                playerHealth -= 10;
                playerHealth = std::max(playerHealth, 0);

                enemy.attackCooldown = 0.8f;
                damageFlashTimer = 0.18f;
            }
        }

        if (playerHealth <= 0)
            running = false;

        // ====================================================
        // WALL RENDERING + Z BUFFER
        // ====================================================

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

            zBuffer[x] = wallDistance;

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

                    int wallColor;

                    if (wallDistance < 3.0f)
                        wallColor = 93;      // bright yellow
                    else if (wallDistance < 6.0f)
                        wallColor = 33;      // yellow
                    else if (wallDistance < 10.0f)
                        wallColor = 31;      // red
                    else if (wallDistance < 18.0f)
                        wallColor = 90;      // dark gray
                    else
                        wallColor = 90;

                    PutChar(frame, x, y, wallChar, wallColor);
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

                    PutChar(frame, x, y, floorChar, 90);
                }
            }
        }

        // ====================================================
        // SPRITES
        // Draw far-to-near so nearby sprites win.
        // ====================================================

        std::vector<int> spriteOrder;

        for (int i = 0; i < static_cast<int>(sprites.size()); i++)
        {
            if (sprites[i].alive)
                spriteOrder.push_back(i);
        }

        std::sort(
            spriteOrder.begin(),
            spriteOrder.end(),
            [&](int a, int b)
            {
                float adx = sprites[a].x - playerX;
                float ady = sprites[a].y - playerY;

                float bdx = sprites[b].x - playerX;
                float bdy = sprites[b].y - playerY;

                return
                    adx * adx + ady * ady >
                    bdx * bdx + bdy * bdy;
            }
        );

        for (int index : spriteOrder)
        {
            DrawEnemySprite(
                frame,
                sprites[index],
                playerX,
                playerY,
                dirX,
                dirY,
                planeX,
                planeY,
                zBuffer
            );
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

                    for (const Sprite& sprite : sprites)
                    {
                        if (
                            sprite.alive &&
                            static_cast<int>(sprite.x) == worldX &&
                            static_cast<int>(sprite.y) == worldY
                        )
                        {
                            symbol = sprite.mapIcon;
                            break;
                        }
                    }
                }

                int miniColor = 90;

                if (symbol == L'#')
                    miniColor = 96;
                else if (symbol == L'E')
                    miniColor = 91;

                PutChar(
                    frame,
                    MINIMAP_X + miniX,
                    MINIMAP_Y + miniY,
                    symbol,
                    miniColor
                );
            }
        }

        int miniPlayerX =
            MINIMAP_X + HALF_MAP;

        int miniPlayerY =
            MINIMAP_Y + HALF_MAP;

        PutChar(frame, miniPlayerX, miniPlayerY, L'P', 92);

        int facingX =
            static_cast<int>(roundf(cosf(playerAngle)));

        int facingY =
            static_cast<int>(roundf(sinf(playerAngle)));

        PutChar(
            frame,
            miniPlayerX + facingX,
            miniPlayerY + facingY,
            L'*',
            93
        );

        // ====================================================
        // WEAPON + CROSSHAIR
        // ====================================================

        DrawWeapon(frame, muzzleTimer > 0.0f);

        // Recoil indicator: crosshair kicks upward briefly.
        int crosshairY =
            SCREEN_HEIGHT / 2 - (recoilTimer > 0.0f ? 1 : 0);

        PutChar(
            frame,
            SCREEN_WIDTH / 2,
            crosshairY,
            L'+',
            97
        );

        if (damageFlashTimer > 0.0f)
        {
            PutChar(frame, 0, 1, L'!', 91);
            PutChar(frame, SCREEN_WIDTH - 1, 1, L'!', 91);
            PutChar(frame, 0, SCREEN_HEIGHT - 1, L'!', 91);
            PutChar(frame, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, L'!', 91);
        }

        // ====================================================
        // HUD
        // ====================================================

        float fps = 1.0f / dt;

        wchar_t hud[SCREEN_WIDTH];

        swprintf_s(
            hud,
            SCREEN_WIDTH,
            L"ASCII3D V7.1 COLOR | HP %d | AMMO %d/%d | KILLS %d | SPACE Shoot | R Reload | WASD | Arrows | ESC",
            playerHealth,
            ammo,
            reserveAmmo,
            kills
        );

        for (
            int i = 0;
            hud[i] != L'\0' &&
            i < SCREEN_WIDTH;
            i++
        )
        {
            PutChar(frame, i, 0, hud[i], 96);
        }

        // ====================================================
        // BUILD TERMINAL FRAME
        // ====================================================

        std::wstring output;

        output.reserve(
            SCREEN_WIDTH * SCREEN_HEIGHT +
            SCREEN_HEIGHT * 2 +
            8
        );

        output += L"\x1b[H";

        int activeColor = -1;

        for (int y = 0; y < SCREEN_HEIGHT; y++)
        {
            for (int x = 0; x < SCREEN_WIDTH; x++)
            {
                int index = y * SCREEN_WIDTH + x;
                int wantedColor = gColors[index];

                if (wantedColor != activeColor)
                {
                    output += L"\x1b[";
                    output += std::to_wstring(wantedColor);
                    output += L"m";
                    activeColor = wantedColor;
                }

                output += frame[index];
            }

            if (y != SCREEN_HEIGHT - 1)
                output += L"\r\n";
        }

        output += L"\x1b[0m";

        WriteVT(output);

        Sleep(8);
    }

    WriteVT(L"\x1b[?25h\x1b[?1049l");

    return 0;
}
