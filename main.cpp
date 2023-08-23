#include <iostream>

#include <fmt/format.h>

#include "raylib.h"
#include "raymath.h"

constexpr int RECT_NUMBER = 4096;
constexpr int ENOUGH = 4096;

struct Enemy {
  float x = 0;
  float y = 0;
  int hp = 0;
  bool alive = false;
};

struct Bullet {
  float x = 0;
  float y = 0;
  float dx = 0;
  float dy = 0;
  int typ = 0;
  bool alive = false;
};

template <typename T> int find_dead(T array[], int len, int start) {
  auto old = start;
  do {
    start = (start + 1) % len;
    if (!array[start].alive) {
      break;
    }
  } while (start != old);
  if (old == start) {
    start = -1;
  }
  return start;
}

int main(int argc, char *argv[]) {
  SetRandomSeed(time(NULL));
  auto win_w = 1280;
  auto win_h = 800;
  auto fps = 60;

  InitWindow(win_w, win_h, "VSRO");

  Rectangle rectangles[RECT_NUMBER];
  Color rect_colors[RECT_NUMBER];
  Enemy enemies[ENOUGH];
  Bullet bullets[ENOUGH];

  for (auto &rect : rectangles) {
    auto x = float(GetRandomValue(-10000, 10000));
    auto y = float(GetRandomValue(-10000, 10000));
    auto size = float(GetRandomValue(400, 1600));
    rect = Rectangle{x - size / 2, y - size / 2, size, size};
  }

  for (auto &color : rect_colors) {
    color = Color{0, uint8_t(128 + GetRandomValue(-64, 64)), 0, 255};
  }

  Vector2 player{720, 400};
  int speed = 2;
  Camera2D camera = {0};
  camera.target = Vector2{player.x, player.y};
  camera.offset = Vector2{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
  camera.rotation = 0;
  camera.zoom = 1;

  SetTargetFPS(fps);
  uint64_t frame_counter = 0;
  int dead_enemy = 0;
  int dead_bullet = 0;

  while (!WindowShouldClose()) {
    auto w = GetScreenWidth();
    auto h = GetScreenHeight();

    if (frame_counter % 4 == 0) {
      if (dead_enemy < 0) {
        dead_enemy = find_dead(enemies, ENOUGH, dead_enemy);
      }
      if (dead_enemy >= 0) {
        int dir = GetRandomValue(1, 4);
        switch (dir) {
        case 1:
          enemies[dead_enemy].x = player.x - w / 2 - 20;
          enemies[dead_enemy].y = GetRandomValue(0, h);
          break;
        case 2:
          enemies[dead_enemy].x = player.x + w / 2 + 20;
          enemies[dead_enemy].y = GetRandomValue(0, h);
          break;
        case 3:
          enemies[dead_enemy].y = player.y - h / 2 - 20;
          enemies[dead_enemy].x = GetRandomValue(0, w);
          break;
        case 4:
          enemies[dead_enemy].y = player.y + h / 2 + 20;
          enemies[dead_enemy].x = GetRandomValue(0, w);
          break;
        default:
          break;
        }
        enemies[dead_enemy].hp = 10;
        enemies[dead_enemy].alive = true;

        dead_enemy = find_dead(enemies, ENOUGH, dead_enemy);
      }
    }

    if (frame_counter % 3 == 0) {
      if (dead_bullet < 0) {
        dead_bullet = find_dead(bullets, ENOUGH, dead_bullet);
      }
      if (dead_bullet >= 0) {
        bullets[dead_bullet].x = player.x;
        bullets[dead_bullet].y = player.y;
        bullets[dead_bullet].dx = GetRandomValue(-10, 10);
        bullets[dead_bullet].dy = GetRandomValue(-10, 10);
        bullets[dead_bullet].typ = GetRandomValue(1, 3);
        bullets[dead_bullet].alive = true;
        dead_bullet = find_dead(bullets, ENOUGH, dead_bullet);
      }
    }

    for (auto &e : enemies) {
      if (e.alive) {
        auto d = Vector2Distance(player, Vector2{e.x, e.y});
        auto dx = (player.x - e.x) / d;
        auto dy = (player.y - e.y) / d;
        e.x += dx;
        e.y += dy;
      }
    }

    for (auto &b : bullets) {
      if (b.alive) {
        auto min_d = std::numeric_limits<float>::max();
        auto min_idx = 0;
        auto counter = 0;
        for (auto &e : enemies) {
          if (e.alive) {
            if (CheckCollisionPointCircle(Vector2{b.x, b.y}, Vector2{e.x, e.y},
                                          16)) {
              b.alive = false;
              e.hp -= 10;
              if (e.hp <= 0) {
                e.alive = false;
              }
              break;
            } else if (b.typ == 2 || b.typ == 3) {
              auto d = Vector2Distance(Vector2{e.x, e.y}, Vector2{b.x, b.y});
              if (d < min_d) {
                min_d = d;
                min_idx = counter;
              }
            }
          }
          counter += 1;
        }
        if (b.alive) {
          if (b.typ == 2) {
            auto dx = 3 * (enemies[min_idx].x - b.x) / min_d;
            auto dy = 3 * (enemies[min_idx].y - b.y) / min_d;
            b.dx += 0.1 * dx;
            b.dy += 0.1 * dy;
          }
          if (b.typ == 3) {
            auto dx = 3 * (enemies[min_idx].x - b.x) / min_d;
            auto dy = 3 * (enemies[min_idx].y - b.y) / min_d;
            b.dx = dx;
            b.dy = dy;
          }
          b.x += b.dx;
          b.y += b.dy;
          if (b.typ == 1 && Vector2Distance(player, Vector2{b.x, b.y}) > 1000) {
            b.alive = false;
          }
        }
      }
    }

    if (IsKeyDown(KEY_LEFT)) {
      player.x -= speed;
    }
    if (IsKeyDown(KEY_RIGHT)) {
      player.x += speed;
    }
    if (IsKeyDown(KEY_UP)) {
      player.y -= speed;
    }
    if (IsKeyDown(KEY_DOWN)) {
      player.y += speed;
    }

    camera.target = Vector2{player.x, player.y};

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);
    for (int i = 0; i < RECT_NUMBER; ++i) {
      DrawRectangleRec(rectangles[i], rect_colors[i]);
    }
    for (int i = 0; i < ENOUGH; ++i) {
      if (enemies[i].alive) {
        DrawCircleV(Vector2{enemies[i].x, enemies[i].y}, 16, RED);
      }
      if (bullets[i].alive) {
        if (bullets[i].typ == 2) {
          DrawPoly(Vector2{bullets[i].x, bullets[i].y}, 3, 8,
                   RAD2DEG * atan2(bullets[i].dx, bullets[i].dy), WHITE);
        } else if(bullets[i].typ == 1) {
          DrawCircleV(Vector2{bullets[i].x, bullets[i].y}, 4, WHITE);
        } else {
          DrawPoly(Vector2{bullets[i].x, bullets[i].y}, 4, 8,
                   RAD2DEG * atan2(bullets[i].dx, bullets[i].dy), WHITE);
        }
      }
    }
    DrawCircle(player.x, player.y, 32, BLUE);
    EndMode2D();
    EndDrawing();
    frame_counter += 1;
  }

  CloseWindow();
  return 0;
}
