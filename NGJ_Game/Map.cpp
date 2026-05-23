#include "Map.h"
#include "AssetManager.h"
#include <math.h>
#include <cstdio>
#include <deque>
#include <queue>
#include <vector>
#include <algorithm>

Map::Map(int screenWidth, int screenHeight) {
    tileWidth = 40;
    tileHeight = 40;

    // 讓地圖大小直接等於螢幕大小
    mapCols = screenWidth / tileWidth;
    mapRows = screenHeight / tileHeight;

    grid.assign(mapRows, std::vector<int>(mapCols, 0));
    sampledColors.assign(mapRows, std::vector<Color>(mapCols, WHITE));

    spawnAtDoor = false;
    debugMode = true;
    for (int i = 0; i < 4; i++) layerKeys[i] = false;

    GenerateDefaultMapFiles();
    InitLayer(DungeonLayer::LAYER_1);
}

// ... (GenerateDefaultMapFiles 與 LoadMapFromImageFile 維持不變) ...

void Map::Update(Vector2 playerMapPos) {
    if (currentLayer == DungeonLayer::VICTORY || currentLayer == DungeonLayer::BOSS_ROOM) return;

    // remember player's map position for drawing contextual hints
    lastPlayerMapPos = playerMapPos;
    float distToKey = sqrtf(powf(playerMapPos.x - keyPos.x, 2) + powf(playerMapPos.y - keyPos.y, 2));
    float distToNextDoor = sqrtf(powf(playerMapPos.x - doorPos.x, 2) + powf(playerMapPos.y - doorPos.y, 2));
    float distToPrevDoor = sqrtf(powf(playerMapPos.x - playerStartPos.x, 2) + powf(playerMapPos.y - playerStartPos.y, 2));

    // update debug distances
    lastDistToKey = distToKey;
    lastDistToDoor = distToNextDoor;
    lastDistToStart = distToPrevDoor;

    // read input: use IsKeyPressed for pickup (single press), IsKeyDown for door/return (hold or press)
    bool spacePressed = IsKeyPressed(KEY_SPACE);
    bool spaceDown = IsKeyDown(KEY_SPACE);
    if (GetTime() < ignoreInputUntil) { spacePressed = spaceDown = false; }
    lastSpacePressed = spacePressed;

    // pickup only on single press
    if (spacePressed) {
        if (distToKey < 45.0f && !layerKeys[(int)currentLayer]) {
            layerKeys[(int)currentLayer] = true;
            // play pickup sound if available
            PlaySound(AssetManager::GetSoundPickup());
        }
    }

    // debug overlay near player: show distances and last space state
    if (debugMode) {
        char buf[256];
        int len = snprintf(buf, sizeof(buf), "dKey=%.1f dDoor=%.1f space=%d keyHave=%d", lastDistToKey, lastDistToDoor, lastSpacePressed ? 1 : 0, PlayerHasKey() ? 1 : 0);
        DrawText(buf, (int)(lastPlayerMapPos.x - 120), (int)(lastPlayerMapPos.y - 80), 12, YELLOW);
    }

    // door/return actions respond to key held or pressed (more tolerant)
    if (spaceDown) {
        if (distToNextDoor < 55.0f && (layerKeys[(int)currentLayer] || currentLayer == DungeonLayer::LAYER_3)) {
            PlaySound(AssetManager::GetSoundExpand());
            AdvanceLayer();
        }
        else if (distToPrevDoor < 55.0f && currentLayer != DungeonLayer::LAYER_1) {
            ReturnLayer();
        }
    }
}

void Map::AdvanceLayer() {
    spawnAtDoor = false;
    if (currentLayer == DungeonLayer::LAYER_1) {
        InitLayer(DungeonLayer::LAYER_2);
    }
    else if (currentLayer == DungeonLayer::LAYER_2) {
        InitLayer(DungeonLayer::LAYER_3);
    }
    else if (currentLayer == DungeonLayer::LAYER_3) {
        InitLayer(DungeonLayer::BOSS_ROOM);
    }
    // set short cooldown to avoid immediate re-trigger
    ignoreInputUntil = GetTime() + 0.3;
}

void Map::ReturnLayer() {
    spawnAtDoor = true; // ensure player spawns at the door when returning to previous layer
    if (currentLayer == DungeonLayer::LAYER_2) InitLayer(DungeonLayer::LAYER_1);
    else if (currentLayer == DungeonLayer::LAYER_3) InitLayer(DungeonLayer::LAYER_2);
    else if (currentLayer == DungeonLayer::BOSS_ROOM) InitLayer(DungeonLayer::LAYER_3);
    ignoreInputUntil = GetTime() + 0.3;
}

void Map::DrawObjects() {
    if (currentLayer != DungeonLayer::LAYER_1) {
        DrawCircleV(playerStartPos, 20.0f, Fade(SKYBLUE, 0.5f));
        DrawText("UP", playerStartPos.x - 10, playerStartPos.y - 8, 10, WHITE);
    }
    // draw door using a texture (fallback to buff icon)
    Texture2D doorTex = AssetManager::GetBuffItemTexture();
    if (doorTex.id != 0) {
        Rectangle src = { 0.0f, 0.0f, (float)doorTex.width, (float)doorTex.height };
        Vector2 destSize = { 48.0f, 72.0f };
        Rectangle dest = { doorPos.x - destSize.x/2.0f, doorPos.y - destSize.y/2.0f, destSize.x, destSize.y };
        Vector2 origin = { destSize.x/2.0f, destSize.y/2.0f };
        Color tint = PlayerHasKey() ? WHITE : MAROON;
        DrawTexturePro(doorTex, src, dest, origin, 0.0f, tint);
    } else {
        Color doorColor = PlayerHasKey() ? Color{0,200,120,255} : MAROON;
        DrawRectangleV({ doorPos.x - 20, doorPos.y - 30 }, { 40, 60 }, doorColor);
    }
    DrawText("DOWN", doorPos.x - 14, doorPos.y - 5, 10, WHITE);

    // draw key using asset texture when not yet picked up
    if (!PlayerHasKey()) {
        Texture2D keyTex = AssetManager::GetItemKeyTexture();
        if (keyTex.id != 0) {
            Rectangle srcK = { 0.0f, 0.0f, (float)keyTex.width, (float)keyTex.height };
            Vector2 kSize = { 36.0f, 36.0f };
            Rectangle destK = { keyPos.x - kSize.x/2.0f, keyPos.y - kSize.y/2.0f, kSize.x, kSize.y };
            Vector2 kOrigin = { kSize.x/2.0f, kSize.y/2.0f };
            DrawTexturePro(keyTex, srcK, destK, kOrigin, 0.0f, WHITE);
        } else {
            DrawCircleV(keyPos, 12.0f, GOLD);
        }
    }

    // contextual prompts when player is near key or door
    float hintDist = 60.0f;
    float dxk = lastPlayerMapPos.x - keyPos.x;
    float dyk = lastPlayerMapPos.y - keyPos.y;
    float dKey = sqrtf(dxk*dxk + dyk*dyk);
    float dxd = lastPlayerMapPos.x - doorPos.x;
    float dyd = lastPlayerMapPos.y - doorPos.y;
    float dDoor = sqrtf(dxd*dxd + dyd*dyd);

    if (dKey < hintDist && !PlayerHasKey()) {
        const char* txt = "PICK UP (SPACE)";
        int tw = MeasureText(txt, 12);
        DrawText(txt, (int)(keyPos.x - tw*0.5f), (int)(keyPos.y - 30), 12, WHITE);
    }

    if (dDoor < hintDist) {
        if (PlayerHasKey()) {
            const char* txt = "ENTER (SPACE)";
            int tw = MeasureText(txt, 12);
            DrawText(txt, (int)(doorPos.x - tw*0.5f), (int)(doorPos.y - 40), 12, WHITE);
        } else {
            const char* txt = "LOCKED";
            int tw = MeasureText(txt, 12);
            DrawText(txt, (int)(doorPos.x - tw*0.5f), (int)(doorPos.y - 40), 12, RED);
        }
    }
}

// Implementations for missing methods (fallbacks and simple rendering)
#include <ctime>
#include <algorithm>

void Map::GenerateDefaultMapFiles() {
    // 關鍵：這裡我們直接使用 class 的成員變數 mapCols/mapRows
    // 確保這些變數在建構子裡已經正確被賦值為 (螢幕解析度 / 40)

    auto save = [&](const std::vector<Color>& data, const char* name) {
        // 'data' is a tile grid of size mapCols x mapRows. Upscale to pixel resolution
        int imgW = mapCols * tileWidth;
        int imgH = mapRows * tileHeight;
        std::vector<Color> pixels(imgW * imgH, BLACK);
        for (int ty = 0; ty < mapRows; ++ty) {
            for (int tx = 0; tx < mapCols; ++tx) {
                Color c = data[ty * mapCols + tx];
                for (int py = 0; py < tileHeight; ++py) {
                    for (int px = 0; px < tileWidth; ++px) {
                        int pxX = tx * tileWidth + px;
                        int pxY = ty * tileHeight + py;
                        pixels[pxY * imgW + pxX] = c;
                    }
                }
            }
        }
        Image img = { (void*)pixels.data(), imgW, imgH, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        ExportImage(img, name);
        };

    auto generateDungeon = [&](const char* filename, bool isBoss) {
        // 使用 mapCols * mapRows 確保生成的圖片就是整個螢幕的像素大小
        std::vector<Color> m(mapCols * mapRows, BLACK);

        // 1. 複雜的迷宮演算法：隨機挖洞 (讓路變多、變複雜)
        for (int y = 2; y < mapRows - 2; y += 2) {
            for (int x = 2; x < mapCols - 2; x += 2) {
                m[y * mapCols + x] = WHITE; // 挖路
                // 隨機往右或往下挖，確保連通性
                if (rand() % 2 == 0) m[y * mapCols + (x + 1)] = WHITE;
                else m[(y + 1) * mapCols + x] = WHITE;
            }
        }

        // 2. 房間生成 (地下城感)
        for (int i = 0; i < 5; i++) {
            int rw = rand() % 8 + 4; int rh = rand() % 8 + 4;
            int rx = rand() % (mapCols - rw - 2) + 1; int ry = rand() % (mapRows - rh - 2) + 1;
            for (int y = ry; y < ry + rh; y++)
                for (int x = rx; x < rx + rw; x++) m[y * mapCols + x] = WHITE;
        }

        // 3. 防卡牆放置：利用亮度偵測
        auto place = [&](Color c) {
            int p;
            int attempts = 0;
            do { p = rand() % (mapCols * mapRows); attempts++; } while (m[p].r < 200 && attempts < 10000);
            m[p] = c;
            };
        place(GREEN); place(BLUE); place(RED);

        save(m, filename);
        };

    generateDungeon("map1.png", false);
    generateDungeon("map2.png", false);
    generateDungeon("map3.png", false);
    generateDungeon("boss.png", true);
}

void Map::LoadMapFromImageFile(const char* fileName) {
    if (!FileExists(fileName)) {
        mapRows = 10; mapCols = 10;
        grid.assign(mapRows, std::vector<int>(mapCols, 0));
        playerStartPos = { (float)tileWidth * 1.5f, (float)tileHeight * 1.5f };
        keyPos = { (float)tileWidth * 3.5f, (float)tileHeight * 3.5f };
        doorPos = { (float)tileWidth * 8.5f, (float)tileHeight * 8.5f };
        return;
    }

    Image img = LoadImage(fileName);
    mapRows = img.height;
    mapCols = img.width;
    grid.assign(mapRows, std::vector<int>(mapCols, 0));
    sampledColors.assign(mapRows, std::vector<Color>(mapCols, {0,0,0,255}));

    // default positions in case PNG lacks markers
    Vector2 defaultSpawn = { (float)tileWidth * 1.5f, (float)tileHeight * 1.5f };
    Vector2 defaultKey = { (float)tileWidth * 3.5f, (float)tileHeight * 3.5f };
    Vector2 defaultDoor = { (float)tileWidth * 8.5f, (float)tileHeight * 8.5f };
    bool foundSpawn = false, foundKey = false, foundDoor = false;

    for (int y = 0; y < mapRows; ++y) {
        for (int x = 0; x < mapCols; ++x) {
            Color p = GetImageColor(img, x, y);
            grid[y][x] = (p.r == 0 && p.g == 0 && p.b == 0) ? 1 : 0;
            if (debugMode) sampledColors[y][x] = p;
            if (!foundSpawn && p.g > 200 && p.r < 50 && p.b < 50) {
                // ensure spawn is not placed inside a wall
                if (grid[y][x] == 0) {
                    playerStartPos = { x * (float)tileWidth + tileWidth/2.0f, y * (float)tileHeight + tileHeight/2.0f };
                    foundSpawn = true;
                } else {
                    // will search for nearest floor later
                    playerStartPos = { (float)x, (float)y };
                    foundSpawn = true; // mark found but will relocate
                }
            }
            if (!foundKey && p.b > 200 && p.r < 50 && p.g < 50) {
                if (grid[y][x] == 0) {
                    keyPos = { x * (float)tileWidth + tileWidth/2.0f, y * (float)tileHeight + tileHeight/2.0f };
                    foundKey = true;
                } else {
                    keyPos = { (float)x, (float)y };
                    foundKey = true;
                }
            }
            if (!foundDoor && p.r > 200 && p.g < 50 && p.b < 50) {
                if (grid[y][x] == 0) {
                    doorPos = { x * (float)tileWidth + tileWidth/2.0f, y * (float)tileHeight + tileHeight/2.0f };
                    foundDoor = true;
                } else {
                    doorPos = { (float)x, (float)y };
                    foundDoor = true;
                }
            }
        }
    }

    // helper: find nearest floor tile to given cell coords (if initial marker fell on wall)
    // local helper: find nearest floor tile to given cell coords (if initial marker fell on wall)
    auto findNearestFloor = [&](int sx, int sy)->Vector2 {
        std::vector<std::vector<char>> vis(mapRows);
        for (int i = 0; i < mapRows; ++i) vis[i].assign(mapCols, 0);
        std::deque<std::pair<int,int>> dq;
        if (sx >= 0 && sx < mapCols && sy >= 0 && sy < mapRows) {
            dq.emplace_back(sx, sy);
            vis[sy][sx] = 1;
        }
        while (!dq.empty()) {
            auto front = dq.front(); dq.pop_front();
            int cx = front.first; int cy = front.second;
            if (grid[cy][cx] == 0) return Vector2{ cx * (float)tileWidth + tileWidth/2.0f, cy * (float)tileHeight + tileHeight/2.0f };
            const int offs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            for (int i=0;i<4;i++){
                int nx = cx + offs[i][0];
                int ny = cy + offs[i][1];
                if (nx < 0 || ny < 0 || nx >= mapCols || ny >= mapRows) continue;
                if (vis[ny][nx]) continue;
                vis[ny][nx] = 1;
                dq.emplace_back(nx, ny);
            }
        }
        // fallback center
        return Vector2{ (float)tileWidth * 1.5f, (float)tileHeight * 1.5f };
    };

    // if marker was placed on wall, relocate to nearest floor
    if (foundSpawn) {
        // if stored as raw cell coords (we signalled with integer-like values), check
        if (playerStartPos.x >= 0 && playerStartPos.x < mapCols && playerStartPos.y >= 0 && playerStartPos.y < mapRows) {
            int sx = (int)playerStartPos.x; int sy = (int)playerStartPos.y;
            if (grid[sy][sx] == 1) playerStartPos = findNearestFloor(sx, sy);
        }
    }
    if (foundKey) {
        if (keyPos.x >= 0 && keyPos.x < mapCols && keyPos.y >= 0 && keyPos.y < mapRows) {
            int sx = (int)keyPos.x; int sy = (int)keyPos.y;
            if (grid[sy][sx] == 1) keyPos = findNearestFloor(sx, sy);
        }
    }
    if (foundDoor) {
        if (doorPos.x >= 0 && doorPos.x < mapCols && doorPos.y >= 0 && doorPos.y < mapRows) {
            int sx = (int)doorPos.x; int sy = (int)doorPos.y;
            if (grid[sy][sx] == 1) doorPos = findNearestFloor(sx, sy);
        }
    }

    // Ensure map connectivity: connect any floor islands so all floor tiles reachable from spawn
    // Determine starting cell (use playerStartPos if valid, otherwise center)
    int startCx = (int)(playerStartPos.x / tileWidth);
    int startCy = (int)(playerStartPos.y / tileHeight);
    if (startCx < 0 || startCx >= mapCols || startCy < 0 || startCy >= mapRows) {
        startCx = mapCols / 2; startCy = mapRows / 2;
    }

    auto computeReachable = [&](std::vector<char>& reachable) {
        reachable.assign(mapRows * mapCols, 0);
        std::queue<std::pair<int,int>>q;
        if (grid[startCy][startCx] == 0) {
            q.emplace(startCx, startCy);
            reachable[startCy*mapCols + startCx] = 1;
        }
        while(!q.empty()){
            auto p = q.front(); q.pop();
            int cx = p.first, cy = p.second;
            const int offs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            for(int i=0;i<4;i++){
                int nx = cx + offs[i][0]; int ny = cy + offs[i][1];
                if (nx<0||ny<0||nx>=mapCols||ny>=mapRows) continue;
                if (reachable[ny*mapCols + nx]) continue;
                if (grid[ny][nx] == 0) {
                    reachable[ny*mapCols + nx] = 1;
                    q.emplace(nx, ny);
                }
            }
        }
    };

    std::vector<char> reachable;
    computeReachable(reachable);

    auto anyUnreachableFloor = [&]() -> bool {
        for (int y=0;y<mapRows;y++) for (int x=0;x<mapCols;x++) if (grid[y][x]==0 && !reachable[y*mapCols + x]) return true;
        return false;
    };

    // Connect islands by carving straight corridors to nearest reachable tile until all connected
    while (anyUnreachableFloor()) {
        // find an unreachable floor tile
        int ux=-1, uy=-1;
        for (int y=0;y<mapRows && ux==-1;y++) for (int x=0;x<mapCols;x++) if (grid[y][x]==0 && !reachable[y*mapCols + x]) { ux=x; uy=y; break; }
        if (ux==-1) break;

        // find nearest reachable tile
        int bestRx=-1, bestRy=-1; int bestDist = INT_MAX;
        for (int y=0;y<mapRows;y++) for (int x=0;x<mapCols;x++) if (reachable[y*mapCols + x]) {
            int d = abs(x-ux) + abs(y-uy);
            if (d < bestDist) { bestDist=d; bestRx=x; bestRy=y; }
        }
        if (bestRx==-1) break;

        // carve horizontal then vertical corridor (L-shaped)
        int x0 = ux, x1 = bestRx;
        if (x0 > x1) std::swap(x0,x1);
        for (int x=x0;x<=x1;x++) grid[uy][x] = 0;
        int y0 = uy, y1 = bestRy;
        if (y0 > y1) std::swap(y0,y1);
        for (int y=y0;y<=y1;y++) grid[y][bestRx] = 0;

        // recompute reachable
        computeReachable(reachable);
    }

    // ensure key/door/spawn are on floor after connectivity
    auto ensureOnFloor = [&](Vector2 &pos){
        int cx = (int)(pos.x / tileWidth);
        int cy = (int)(pos.y / tileHeight);
        if (cx<0||cy<0||cx>=mapCols||cy>=mapRows || grid[cy][cx]==1) {
            Vector2 nf = findNearestFloor(cx, cy);
            pos = nf;
        }
    };
    ensureOnFloor(playerStartPos);
    ensureOnFloor(keyPos);
    ensureOnFloor(doorPos);

    // fallback to defaults if markers not found in image
    if (!foundSpawn) playerStartPos = defaultSpawn;
    if (!foundKey) keyPos = defaultKey;
    if (!foundDoor) doorPos = defaultDoor;

    UnloadImage(img);
}

void Map::InitLayer(DungeonLayer layer) {
    currentLayer = layer;
    if (layer == DungeonLayer::LAYER_1) LoadMapFromImageFile("map1.png");
    else if (layer == DungeonLayer::LAYER_2) LoadMapFromImageFile("map2.png");
    else if (layer == DungeonLayer::LAYER_3) LoadMapFromImageFile("map3.png");
    else if (layer == DungeonLayer::BOSS_ROOM) LoadMapFromImageFile("boss.png");
}

void Map::SetDebugMode(bool enabled) {
    debugMode = enabled;
}

void Map::DrawBaseMap() {
    Color wallColor = { 55, 55, 65, 255 };
    Color floorColor = { 25, 25, 30, 255 };
    if (currentLayer == DungeonLayer::BOSS_ROOM) { wallColor = { 90, 35, 35, 255 }; floorColor = { 40, 20, 20, 255 }; }
    for (int y = 0; y < mapRows; ++y) {
        for (int x = 0; x < mapCols; ++x) {
            Rectangle r = { x * (float)tileWidth, y * (float)tileHeight, (float)tileWidth, (float)tileHeight };
            if (grid[y][x] == 1) DrawRectangleRec(r, wallColor); else DrawRectangleRec(r, floorColor);
            if (debugMode) {
                // draw sampled pixel color as small overlay in corner of tile for debugging
                Color sc = sampledColors[y][x];
                Rectangle s = { r.x + 2, r.y + 2, 12, 12 };
                DrawRectangleRec(s, sc);
                DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, Fade(RAYWHITE, 0.03f));
            }
        }
    }
}

bool Map::IsWall(float worldX, float worldY) const {
    int cx = (int)(worldX / tileWidth);
    int cy = (int)(worldY / tileHeight);
    if (cx < 0 || cy < 0 || cy >= mapRows || cx >= mapCols) return true;
    return grid[cy][cx] == 1;
}
