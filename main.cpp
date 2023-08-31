#include <iostream>

#include <fmt/format.h>

#include "raylib.h"
#include "raymath.h"

constexpr int RECT_NUMBER = 4096;
constexpr int ENOUGH = 4096;
constexpr double phi = 1.61803398875;

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
  int damage = 0;
  int close_encounters[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  bool alive = false;
};

struct Rocket {
  Vector2 pos{0, 0};
  Vector2 dv{0, 0};
  bool alive = false;
};

struct Experience {
  Vector2 pos{0, 0};
  int value = 0;
  int typ = 0;
  bool alive = false;
};

struct Item {
  Vector2 pos{0, 0};
  int typ = 0;
  bool alive = false;
};

struct Boss {
  Vector2 pos{0, 0};
  int hp = 1000000;
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
  Item items[ENOUGH];
  Rocket rocket;
  Boss boss;
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
  int dead_item = 0;

  auto drop_xp = [&](Enemy &e) {
    if (dead_experience < 0) {
      dead_experience = find_dead(experiences, ENOUGH, dead_experience);
    }
    if (dead_experience >= 0) {
      auto &experience = experiences[dead_experience];
      experience.alive = true;
      experience.pos = e.pos;
      experience.value = e.init_hp / 10;
      experience.typ = 0;
    } else {
      experiences[0].value += e.init_hp / 10;
      experiences[0].typ = 1;
    }
    dead_experience = find_dead(experiences, ENOUGH, dead_experience);
  };

  bool game_over = false;
  bool pause = true;

  auto morda_r = LoadTexture("morda_r.png");
  auto morda_l = LoadTexture("morda_l.png");
  auto morda_o = LoadTexture("morda_o.png");
  auto boss_texture = LoadTexture("boss.png");
  auto enemy_texture = LoadTexture("enemy.png");
  int player_dir = 0;
  int player_launches = 0;
  if (!IsTextureReady(morda_o) || !IsTextureReady(morda_l) ||
      !IsTextureReady(morda_r)) {
    player_dir = -1;
  }

  while (!WindowShouldClose()) {
    auto w = GetScreenWidth();
    auto h = GetScreenHeight();

    if (!game_over && !pause) {
      player_speed = 2 + player_level;
      int freq = 30 - 30 * ((frame_counter % 3600) / 3600.0);
      if (frame_counter % std::max(1, freq) == 0) {
        for (int i = 0; i < player_level + 1; ++i) {
          if (dead_enemy < 0) {
            dead_enemy = find_dead(enemies, ENOUGH, dead_enemy);
          }
          if (dead_enemy >= 0) {
            auto &enemy = enemies[dead_enemy];
            int dir = GetRandomValue(1, 4);
            switch (dir) {
            case 1:
              enemy.pos.x = player.x - w / 2 - 20;
              enemy.pos.y = GetRandomValue(player.y - h / 2, player.y + h / 2);
              break;
            case 2:
              enemy.pos.x = player.x + w / 2 + 20;
              enemy.pos.y = GetRandomValue(player.y - h / 2, player.y + h / 2);
              break;
            case 3:
              enemy.pos.y = player.y - h / 2 - 20;
              enemy.pos.x = GetRandomValue(player.x - w / 2, player.x + w / 2);
              break;
            case 4:
              enemy.pos.y = player.y + h / 2 + 20;
              enemy.pos.x = GetRandomValue(player.x - w / 2, player.x + w / 2);
              break;
            default:
              break;
            }
            enemy.init_hp = GetRandomValue(5, 50);
            enemy.hp = enemies[dead_enemy].init_hp;
            enemy.alive = true;

            dead_enemy = find_dead(enemies, ENOUGH, dead_enemy);
          }
        }
      }

      if (frame_counter % (player_level >= 10 ? 1 : (10 - player_level)) == 0) {
        for (int i = 0; i < player_level + 1; ++i) {
          if (dead_bullet < 0) {
            dead_bullet = find_dead(bullets, ENOUGH, dead_bullet);
          }
          if (dead_bullet >= 0) {
            auto &bullet = bullets[dead_bullet];
            bullet.pos = player;
            bullet.dv.x = GetRandomValue(-10, 10);
            bullet.dv.y = GetRandomValue(-10, 10);
            bullet.typ = GetRandomValue(1, 4);
            if (bullet.typ == 4) {
              if (GetRandomValue(0, 1)) {
                bullet.pos.x += (-1 * GetRandomValue(-1, 1)) * 64;
                bullet.dv.x = 0;
              } else {
                bullet.pos.y += (-1 * GetRandomValue(-1, 1)) * 64;
                bullet.dv.y = 0;
              }
            }
            switch (bullet.typ) {
            case 1:
              bullet.damage = 10;
              break;
            case 2:
              bullet.damage = 50;
              break;
            case 3:
              bullet.damage = 20;
              break;
            case 4:
              bullet.damage = 15;
              break;
            default:
              break;
            }
            bullet.alive = true;
            bullet.lifetime = bullet.typ == 3 ? 3 : 1;
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
          if (player_dir >= 0) {
            player_launches = 30;
          }
        }
      }

      if (frame_counter % 600 == 0) {
        if (dead_item < 0) {
          dead_item = find_dead(items, ENOUGH, dead_item);
        }
        if (dead_item >= 0) {
          auto &item = items[dead_item];
          item.pos = Vector2{(float)GetRandomValue(-10000, 10000),
                             (float)GetRandomValue(-10000, 10000)};
          item.typ = 1;
          item.alive = true;
          dead_item = find_dead(items, ENOUGH, dead_item);
        }
      }

      if (!boss.alive && (frame_counter % (60 * 60 * 10) == 0)) {
        boss.alive = true;
        boss.hp = 1000000;
        boss.pos = Vector2{10000, 10000};
      }

      for (auto &e : enemies) {
        if (e.alive) {
          auto d = Vector2Distance(player, e.pos);
          if (CheckCollisionCircles(Vector2{player.x, player.y}, 32, e.pos,
                                    16)) {
            player_hp -= 1;
            if (player_hp <= 0) {
              game_over = true;
            }
          }
          auto dx =
              std::max(1.0, player_level / 3.0) * (player.x - e.pos.x) / d;
          auto dy =
              std::max(1.0, player_level / 3.0) * (player.y - e.pos.y) / d;
          e.pos.x += dx;
          e.pos.y += dy;
        }
      }

      for (auto &e : experiences) {
        if (e.alive) {
          auto d = Vector2Distance(player, e.pos);
          if (CheckCollisionCircles(player, 32 + pow(1.3, player_level), e.pos,
                                    8)) {
            player_experience += e.value;
            e.alive = false;
            if (player_experience >= pow(phi, player_level)) {
              player_level = log2(player_experience) / log2(phi);
            }
          }
        }
      }

      for (auto &i : items) {
        if (i.alive) {
          auto d = Vector2Distance(player, i.pos);
          if (CheckCollisionCircles(player, 32, i.pos, 16)) {
            for (auto &e : experiences) {
              if (e.alive) {
                player_experience += e.value;
                e.alive = false;
                if (player_experience >= pow(phi, player_level)) {
                  player_level = log2(player_experience) / log2(phi);
                }
              }
            }
            i.alive = false;
          }
        }
      }

      for (auto &b : bullets) {
        if (b.alive && boss.alive) {
          if (CheckCollisionPointCircle(b.pos, boss.pos, 64)) {
            boss.hp -= b.damage;
            b.lifetime -= 1;
            if (b.lifetime <= 0) {
              b.alive = false;
            }
            if (boss.hp <= 0) {
              boss.alive = false;
            } else {
              boss.pos = Vector2Add(
                  boss.pos,
                  Vector2Scale(Vector2Subtract(player, boss.pos), -0.001));
            }
          }
        }
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
                enemies[e].hp -= b.damage;
                if (enemies[e].hp <= 0) {
                  enemies[e].alive = false;
                  drop_xp(enemies[e]);
                } else {
                  enemies[e].pos =
                      Vector2Add(enemies[e].pos, Vector2Scale(b.dv, -1.0));
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
                  e.hp -= b.damage;
                  if (e.hp <= 0) {
                    e.alive = false;
                    drop_xp(e);
                  } else {
                    e.pos = Vector2Add(e.pos, Vector2Scale(b.dv, -1.0));
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
                } else if (b.typ == 4) {
                  auto d = Vector2Distance(player, b.pos);
                  auto dx = (player.x - b.pos.x) / d;
                  auto dy = (player.y - b.pos.y) / d;
                  if (d > 128) {
                    b.dv.x = (-10 * dy + 0.1 * dx) / 2;
                    b.dv.y = (10 * dx + 0.1 * dy) / 2;
                  } else if (d < 128) {
                    b.dv.x = (-10 * dy - 0.1 * dx) / 2;
                    b.dv.y = (10 * dx - 0.1 * dy) / 2;
                  }
                  b.dv.x = std::min(10.0f, b.dv.x);
                  b.dv.y = std::min(10.0f, b.dv.y);
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
              auto dx = (3 + player_level * 0.1f) *
                        (enemies[min_idx].pos.x - b.pos.x) / min_d;
              auto dy = (3 + player_level * 0.1f) *
                        (enemies[min_idx].pos.y - b.pos.y) / min_d;
              b.dv = Vector2Scale(Vector2Add(b.dv, Vector2{dx, dy}), 0.5);
            }
            b.pos = Vector2Add(b.pos, b.dv);
            if (Vector2Distance(player, b.pos) > 1000) {
              b.alive = false;
            }
          }
        }
      }

      if (rocket.alive) {
        if (boss.alive) {
          if (CheckCollisionPointCircle(rocket.pos, boss.pos, 64)) {
            boss.hp -= 1000;
            if (boss.hp <= 0) {
              boss.alive = false;
            }
          }
        }
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
            auto d =
                Vector2Distance(rocket.pos, enemies[rocket_target_idx].pos);
            auto dx = (player_level + 5) *
                      (enemies[rocket_target_idx].pos.x - rocket.pos.x) / d;
            auto dy = (player_level + 5) *
                      (enemies[rocket_target_idx].pos.y - rocket.pos.y) / d;
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

      if (boss.alive) {
        auto d = Vector2Distance(player, boss.pos);
        if (d < 64) {
          game_over = true;
        } else {
          auto dx = (player.x - boss.pos.x) / d;
          auto dy = (player.y - boss.pos.y) / d;
          boss.pos.x += ((player_level + 1) / 3) * dx;
          boss.pos.y += ((player_level + 1) / 3) * dy;
        }
      }

      if (IsKeyDown(KEY_LEFT)) {
        player_dir = 0;
        player.x -= player_speed;
      }
      if (IsKeyDown(KEY_RIGHT)) {
        player_dir = 1;
        player.x += player_speed;
      }
      if (IsKeyDown(KEY_UP)) {
        player.y -= player_speed;
      }
      if (IsKeyDown(KEY_DOWN)) {
        player.y += player_speed;
      }
      auto move = GetMouseWheelMove();
      camera.zoom += 0.05 * move;
      camera.zoom = std::max(0.0f, camera.zoom);
      if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        camera.zoom = 1.0f;
      }

      if (player.x < -10000) {
        player.x += 20000;
      }
      if (player.x > 10000) {
        player.x -= 20000;
      }
      if (player.y < -10000) {
        player.y += 20000;
      }
      if (player.y > 10000) {
        player.y -= 20000;
      }
    }

    if (IsKeyPressed(KEY_SPACE)) {
      if (game_over) {
        game_over = false;
        pause = false;
        frame_counter = 0;
        player_experience = 0;
        player_level = 0;
        player_hp = 1000;
        for (auto &e : enemies) {
          e.alive = false;
        }
        for (auto &b : bullets) {
          b.alive = false;
        }
        for (auto &e : experiences) {
          e.alive = false;
        }
        rocket.alive = false;
        player = {0, 0};
      } else {
        pause = !pause;
      }
    }

    camera.target = Vector2{player.x, player.y};

    BeginDrawing();
    ClearBackground(LIME);
    BeginMode2D(camera);
    for (int i = 0; i < RECT_NUMBER; ++i) {
      DrawRectangleRec(rectangles[i], rect_colors[i]);
    }
    DrawLineEx({-10000, -10000}, {-10000, 10000}, 5, YELLOW);
    DrawLineEx({10000, -10000}, {10000, 10000}, 5, YELLOW);
    DrawLineEx({-10000, -10000}, {10000, -10000}, 5, YELLOW);
    DrawLineEx({10000, 10000}, {-10000, 10000}, 5, YELLOW);
    for (int i = 0; i < ENOUGH; ++i) {
      if (experiences[i].alive) {
        DrawPoly(experiences[i].pos, 6, 8, 0,
                 experiences[i].typ ? PINK : SKYBLUE);
      }
    }
    for (int i = 0; i < ENOUGH; ++i) {
      if (items[i].alive) {
        DrawPoly(items[i].pos, 4, 16, 0, items[i].typ ? GOLD : GOLD);
      }
    }
    for (int i = 0; i < ENOUGH; ++i) {
      if (enemies[i].alive) {
        if (!IsTextureReady(enemy_texture)) {
          DrawCircleV(enemies[i].pos, 16, RED);
        } else {
          DrawTextureV(enemy_texture, enemies[i].pos, WHITE);
        }
      }
    }
    for (int i = 0; i < ENOUGH; ++i) {
      if (bullets[i].alive) {
        if (bullets[i].typ == 2) {
          DrawPoly(bullets[i].pos, 3, 8,
                   RAD2DEG * atan2(bullets[i].dv.x, bullets[i].dv.y), WHITE);
        } else if (bullets[i].typ == 1) {
          DrawCircleV(bullets[i].pos, 4, WHITE);
        } else if (bullets[i].typ == 4) {
          DrawPoly(bullets[i].pos, 6, 8, 0, WHITE);
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
    if (boss.alive) {
      if (IsTextureReady(boss_texture)) {
        DrawTextureV(boss_texture, Vector2Subtract(boss.pos, {64, 64}), WHITE);
      } else {
        DrawCircleV(boss.pos, 64, MAROON);
      }
      int wg = boss.hp / 1000000.0 * 128;
      int wr = 128 - wg;
      DrawRectangle(boss.pos.x - 64, boss.pos.y - 70, wg, 5, GREEN);
      DrawRectangle(boss.pos.x - 64 + wg, boss.pos.y - 70, wr, 5, RED);
    }
    if (player_dir >= 0) {
      DrawTexture(player_launches > 1 ? morda_o : (player_dir ? morda_r : morda_l),
                  player.x - 32, player.y - 32, WHITE);
      if (player_launches > 0) {
        player_launches -= 1;
      }
    } else {
      DrawCircle(player.x, player.y, 32, BLUE);
    }
    int wg = player_hp / (1000.0 + 100 * player_level) * 64;
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
    int wc = (player_experience - pow(phi, player_level)) /
             pow(phi, player_level) * win_w;
    DrawRectangle(0, 0, wc, 10, SKYBLUE);
    DrawRectangle(wc, 0, win_w - wc, 10, BLUE);
    if (game_over) {
      DrawText("GAME OVER", (w - MeasureText("GAME OVER", 72)) / 2,
               GetScreenHeight() / 2 - 36, 72, BLACK);
    }
    if (pause) {
      DrawText("PAUSE", (w - MeasureText("PAUSE", 72)) / 2,
               GetScreenHeight() / 2 - 36, 72, BLACK);
    }
    EndDrawing();

    if (!game_over && !pause) {
      frame_counter += 1;
      player_hp = std::min(uint64_t(player_hp + 1), 1000 + 100 * player_level);
    }
  }

  CloseWindow();
  return 0;
}
