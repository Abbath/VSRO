#include <iostream>

#include <fmt/format.h>

#include "raylib.h"
#include "raymath.h"

constexpr int RECT_NUMBER = 4096;
constexpr int ENOUGH = 4096;

struct Enemy {
  Vector2 pos{0, 0};
  int hp = 0;
  int init_hp = 0;
  bool alive = false;
};

struct Bullet {
  Vector2 pos{0, 0};
  Vector2 dv{0, 0};
  int typ = 0;
  int lifetime = 0;
  int close_encounters[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  bool alive = false;
};

struct Rocket {
  Vector2 pos{0, 0};
  Vector2 dv{0, 0};
  bool alive = false;
};

struct Experience {
  Vector2 pos;
  int value = 0;
  int typ = 0;
  bool alive = false;
};

template <typename T> int find_dead(T array[], int len, int start) {
  if (start == -1) {
    start = 0;
  }
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
  auto win_w = 1000;
  auto win_h = 1000;
  auto fps = 60;

  auto inside_the_field = [=](Vector2 p, Enemy e) {
    return e.pos.x > p.x - win_w / 2 && e.pos.y > p.y - win_h / 2 &&
           e.pos.x < p.x + win_w / 2 && e.pos.y < p.y + win_h / 2;
  };

  InitWindow(win_w, win_h, "VSRO");

  Rectangle rectangles[RECT_NUMBER];
  Color rect_colors[RECT_NUMBER];
  Enemy enemies[ENOUGH];
  Bullet bullets[ENOUGH];
  Experience experiences[ENOUGH];
  Rocket rocket;
  int rocket_exploded = 0;
  bool rocket_target_locked = false;
  int rocket_target_idx = 0;

  for (auto &rect : rectangles) {
    auto x = float(GetRandomValue(-10000, 10000));
    auto y = float(GetRandomValue(-10000, 10000));
    auto size = float(GetRandomValue(400, 800));
    rect = Rectangle{x - size / 2, y - size / 2, size, size};
  }

  for (auto &color : rect_colors) {
    color = Color{0, uint8_t(128 + GetRandomValue(-64, 64)), 0, 255};
  }

  Vector2 player{720, 400};
  int player_hp = 1000;
  int player_speed = 2;
  uint64_t player_experience = 0;
  uint64_t player_level = 0;
  Camera2D camera = {0};
  camera.target = Vector2{player.x, player.y};
  camera.offset = Vector2{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
  camera.rotation = 0;
  camera.zoom = 1;

  SetTargetFPS(fps);
  uint64_t frame_counter = 0;
  int dead_enemy = 0;
  int dead_bullet = 0;
  int dead_experience = 0;

  auto drop_xp = [&](Enemy &e) {
    if (dead_experience < 0) {
      dead_experience = find_dead(experiences, ENOUGH, dead_experience);
    }
    if (dead_experience >= 0) {
      experiences[dead_experience].alive = true;
      experiences[dead_experience].pos = e.pos;
      experiences[dead_experience].value = e.init_hp;
      experiences[dead_experience].typ = 0;
    } else {
      experiences[0].value += e.init_hp;
      experiences[0].typ = 1;
    }
    dead_experience = find_dead(experiences, ENOUGH, dead_experience);
  };

  while (!WindowShouldClose()) {
    auto w = GetScreenWidth();
    auto h = GetScreenHeight();
    player_speed = 2 + player_level;
    int freq = 30 - 30 * ((frame_counter % 3600) / 3600.0);
    if (frame_counter % std::max(1, freq) == 0) {
      for (int i = 0; i < player_level + 1; ++i) {
        if (dead_enemy < 0) {
          dead_enemy = find_dead(enemies, ENOUGH, dead_enemy);
        }
        if (dead_enemy >= 0) {
          int dir = GetRandomValue(1, 4);
          switch (dir) {
          case 1:
            enemies[dead_enemy].pos.x = player.x - w / 2 - 20;
            enemies[dead_enemy].pos.y = GetRandomValue(0, h);
            break;
          case 2:
            enemies[dead_enemy].pos.x = player.x + w / 2 + 20;
            enemies[dead_enemy].pos.y = GetRandomValue(0, h);
            break;
          case 3:
            enemies[dead_enemy].pos.y = player.y - h / 2 - 20;
            enemies[dead_enemy].pos.x = GetRandomValue(0, w);
            break;
          case 4:
            enemies[dead_enemy].pos.y = player.y + h / 2 + 20;
            enemies[dead_enemy].pos.x = GetRandomValue(0, w);
            break;
          default:
            break;
          }
          enemies[dead_enemy].init_hp = GetRandomValue(5, 50);
          enemies[dead_enemy].hp = enemies[dead_enemy].init_hp;
          enemies[dead_enemy].alive = true;

          dead_enemy = find_dead(enemies, ENOUGH, dead_enemy);
        }
      }
    }

    if (frame_counter % (player_level >= 10 ? 1 : 10 - player_level) == 0) {
      for (int i = 0; i < player_level + 1; ++i) {
        if (dead_bullet < 0) {
          dead_bullet = find_dead(bullets, ENOUGH, dead_bullet);
        }
        if (dead_bullet >= 0) {
          bullets[dead_bullet].pos = player;
          bullets[dead_bullet].dv.x = GetRandomValue(-10, 10);
          bullets[dead_bullet].dv.y = GetRandomValue(-10, 10);
          bullets[dead_bullet].typ = GetRandomValue(1, 3);
          bullets[dead_bullet].alive = true;
          bullets[dead_bullet].lifetime = bullets[dead_bullet].typ == 3 ? 3 : 1;
          dead_bullet = find_dead(bullets, ENOUGH, dead_bullet);
        }
      }
    }

    if (frame_counter % (1200 / (player_level + 1)) == 0) {
      if (!rocket.alive) {
        rocket.pos = player;
        rocket.dv = Vector2{(float)GetRandomValue(-10, 10),
                            (float)GetRandomValue(-10, 10)};
        rocket.alive = true;
      }
    }

    for (auto &e : enemies) {
      if (e.alive) {
        auto d = Vector2Distance(player, e.pos);
        if (CheckCollisionCircles(Vector2{player.x, player.y}, 32, e.pos, 16)) {
          player_hp -= 1;
          if (player_hp <= 0) {
            player_hp = 1000;
          }
        }
        auto dx = (player.x - e.pos.x) / d;
        auto dy = (player.y - e.pos.y) / d;
        e.pos.x += dx;
        e.pos.y += dy;
      }
    }

    for (auto &e : experiences) {
      if (e.alive) {
        auto d = Vector2Distance(player, e.pos);
        if (CheckCollisionCircles(player, 32 + player_level * 2, e.pos, 8)) {
          player_experience += e.value;
          e.alive = false;
          if (player_experience >= (1 << player_level)) {
            player_level = log2(player_experience);
          }
        }
      }
    }

    for (auto &b : bullets) {
      if (b.alive) {
        bool found_close = false;
        auto min_d = std::numeric_limits<float>::max();
        auto min_idx = 0;
        auto counter = 0;
        auto encounter_counter = 0;
        for (auto e : b.close_encounters) {
          if (e > 0 && enemies[e].alive) {
            if (CheckCollisionPointCircle(b.pos, enemies[e].pos, 16)) {
              b.lifetime -= 1;
              if (b.lifetime <= 0) {
                b.alive = false;
              }
              switch (b.typ) {
              case 1:
                enemies[e].hp -= 10;
                break;
              case 2:
                enemies[e].hp -= 50;
                break;
              case 3:
                enemies[e].hp -= 20;
                break;
              default:
                break;
              }
              if (enemies[e].hp <= 0) {
                enemies[e].alive = false;
                drop_xp(enemies[e]);
              }
              break;
            }
            if (b.alive) {
              min_d = Vector2Distance(b.pos, enemies[e].pos);
              min_idx = e;
              found_close = true;
              break;
            }
          }
        }
        if (!found_close) {
          for (auto &e : enemies) {
            if (e.alive) {
              if (CheckCollisionPointCircle(b.pos, e.pos, 16)) {
                b.lifetime -= 1;
                if (b.lifetime <= 0) {
                  b.alive = false;
                }
                switch (b.typ) {
                case 1:
                  e.hp -= 10;
                  break;
                case 2:
                  e.hp -= 50;
                  break;
                case 3:
                  e.hp -= 20;
                  break;
                default:
                  break;
                }
                if (e.hp <= 0) {
                  e.alive = false;
                  drop_xp(e);
                }
                break;
              } else if (b.typ == 2 || b.typ == 3) {
                auto d = Vector2Distance(e.pos, b.pos);
                if (d < min_d && inside_the_field(player, e)) {
                  min_d = d;
                  min_idx = counter;
                  b.close_encounters[encounter_counter] = counter;
                  encounter_counter = (encounter_counter + 1) % 10;
                }
              }
            }
            counter += 1;
          }
        }
        if (b.alive) {
          if (b.typ == 2) {
            auto dx = 3 * (enemies[min_idx].pos.x - b.pos.x) / min_d;
            auto dy = 3 * (enemies[min_idx].pos.y - b.pos.y) / min_d;
            b.dv.x += 0.1 * dx;
            b.dv.y += 0.1 * dy;
          }
          if (b.typ == 3) {
            auto dx = 3 * (enemies[min_idx].pos.x - b.pos.x) / min_d;
            auto dy = 3 * (enemies[min_idx].pos.y - b.pos.y) / min_d;
            b.dv = Vector2Scale(Vector2Add(b.dv, Vector2{dx, dy}), 0.5);
          }
          b.pos = Vector2Add(b.pos, b.dv);
          if (b.typ == 1 && Vector2Distance(player, b.pos) > 1000) {
            b.alive = false;
          }
        }
      }
    }

    if (rocket.alive) {
      if (rocket_target_locked && enemies[rocket_target_idx].alive) {
        if (CheckCollisionPointCircle(rocket.pos,
                                      enemies[rocket_target_idx].pos, 16)) {
          for (auto &e1 : enemies) {
            if (e1.alive) {
              if (Vector2Distance(rocket.pos, e1.pos) <= 500) {
                e1.alive = false;
                drop_xp(e1);
              }
            }
          }
          rocket.alive = false;
          rocket_target_locked = false;
          rocket_exploded = 6;
        }
        if (rocket.alive && enemies[rocket_target_idx].alive) {
          auto d = Vector2Distance(rocket.pos, enemies[rocket_target_idx].pos);
          auto dx = 5 * (enemies[rocket_target_idx].pos.x - rocket.pos.x) / d;
          auto dy = 5 * (enemies[rocket_target_idx].pos.y - rocket.pos.y) / d;
          rocket.dv = Vector2Clamp(
              Vector2Scale(Vector2Add(rocket.dv, Vector2{dx, dy}), 0.5),
              Vector2{-10, -10}, Vector2{10, 10});
          rocket.pos = Vector2Add(rocket.pos, rocket.dv);
        }
      } else {
        rocket_target_locked = false;
        auto min_d = std::numeric_limits<float>::max();
        auto min_idx = 0;
        auto counter = 0;
        for (auto &e : enemies) {
          if (e.alive) {
            if (CheckCollisionPointCircle(rocket.pos, e.pos, 16)) {
              for (auto &e1 : enemies) {
                if (e1.alive) {
                  if (Vector2Distance(rocket.pos, e1.pos) <= 500) {
                    e1.alive = false;
                    drop_xp(e1);
                  }
                }
              }
              rocket.alive = false;
              rocket_exploded = 6;
              break;
            }
            auto d = Vector2Distance(rocket.pos, e.pos);
            if (d < min_d) {
              min_d = d;
              min_idx = counter;
            }
          }
          counter += 1;
        }
        if (rocket.alive) {
          auto dx = 5 * (enemies[min_idx].pos.x - rocket.pos.x) / min_d;
          auto dy = 5 * (enemies[min_idx].pos.y - rocket.pos.y) / min_d;
          rocket.dv = Vector2Clamp(
              Vector2Scale(Vector2Add(rocket.dv, Vector2{dx, dy}), 0.5),
              Vector2{-10, -10}, Vector2{10, 10});
          rocket.pos = Vector2Add(rocket.pos, rocket.dv);
          rocket_target_locked = true;
          rocket_target_idx = min_idx;
        }
      }
    }

    if (IsKeyDown(KEY_LEFT)) {
      player.x -= player_speed;
    }
    if (IsKeyDown(KEY_RIGHT)) {
      player.x += player_speed;
    }
    if (IsKeyDown(KEY_UP)) {
      player.y -= player_speed;
    }
    if (IsKeyDown(KEY_DOWN)) {
      player.y += player_speed;
    }

    camera.target = Vector2{player.x, player.y};

    BeginDrawing();
    ClearBackground(LIME);
    BeginMode2D(camera);
    for (int i = 0; i < RECT_NUMBER; ++i) {
      DrawRectangleRec(rectangles[i], rect_colors[i]);
    }
    for (int i = 0; i < ENOUGH; ++i) {
      if (experiences[i].alive) {
        DrawPoly(experiences[i].pos, 6, 8, 0,
                 experiences[i].typ ? PINK : SKYBLUE);
      }
      if (enemies[i].alive) {
        DrawCircleV(enemies[i].pos, 16, RED);
      }
      if (bullets[i].alive) {
        if (bullets[i].typ == 2) {
          DrawPoly(bullets[i].pos, 3, 8,
                   RAD2DEG * atan2(bullets[i].dv.x, bullets[i].dv.y), WHITE);
        } else if (bullets[i].typ == 1) {
          DrawCircleV(bullets[i].pos, 4, WHITE);
        } else {
          DrawPoly(bullets[i].pos, 4, 8,
                   RAD2DEG * atan2(bullets[i].dv.x, bullets[i].dv.y), WHITE);
        }
      }
    }
    if (rocket_exploded) {
      DrawCircleV(rocket.pos, 500, WHITE);
      rocket_exploded -= 1;
    }
    DrawCircle(player.x, player.y, 32, BLUE);
    int wg = player_hp / 1000.0 * 64;
    int wr = 64 - wg;
    DrawRectangle(player.x - 32, player.y - 40, wg, 5, GREEN);
    DrawRectangle(player.x - 32 + wg, player.y - 40, wr, 5, RED);
    if (rocket.alive) {
      DrawPoly(rocket.pos, 3, 16, RAD2DEG * atan2(rocket.dv.x, rocket.dv.y),
               ORANGE);
      DrawLineV(rocket.pos, enemies[rocket_target_idx].pos, ORANGE);
    }
    EndMode2D();
    DrawText(fmt::format("{:02}:{:02}\nLevel: {}\nXP: {}", frame_counter / 3600,
                         frame_counter % 3600 / 60, player_level,
                         player_experience)
                 .c_str(),
             10, 20, 30, WHITE);
    int wc = (player_experience - (1 << player_level)) /
             double(1 << player_level) * win_w;
    DrawRectangle(0, 0, wc, 10, SKYBLUE);
    DrawRectangle(wc, 0, win_w - wc, 10, BLUE);
    EndDrawing();
    frame_counter += 1;
    player_hp = std::min(player_hp + 1, 1000);
  }

  CloseWindow();
  return 0;
}
