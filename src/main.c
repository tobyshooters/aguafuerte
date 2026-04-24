#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "cli.h"
#include "db.h"
#include "eval.h"
#include "glyph.h"

#define DEFAULT_H 64
#define DEFAULT_W 128

#define SCALE 4
#define MAX_VIEW_H 256
#define SCROLL_STEP 8
#define INPUT_ROW_H (FONT_HEIGHT + 2 + ROW_GAP)

static int scroll_y = 0;
static int view_w = 0;
static int view_h = 0;
static bool show_grid = false;

static void
content_bounds(Database* db, int* w, int* h)
{
  int x0, y0, x1, y1;
  image_bounds(&db->img, &x0, &y0, &x1, &y1);
  *w = x1 + 2;
  *h = y1 + 2;
  if (*w < DEFAULT_W) {
    *w = DEFAULT_W;
  }
  if (*h < DEFAULT_H) {
    *h = DEFAULT_H;
  }
}

static void
update_view(Database* db)
{
  content_bounds(db, &view_w, &view_h);
  view_h += INPUT_ROW_H;
  if (view_h > MAX_VIEW_H) {
    view_h = MAX_VIEW_H;
  }
}

static void
clamp_scroll(Database* db)
{
  int content_h = db->img.height + INPUT_ROW_H;
  int max = content_h - view_h;
  if (max < 0) {
    max = 0;
  }
  if (scroll_y > max) {
    scroll_y = max;
  }
  if (scroll_y < 0) {
    scroll_y = 0;
  }
}

static SDL_Texture*
remake_texture(SDL_Renderer* renderer, SDL_Texture* old, Database* db)
{
  if (old) {
    SDL_DestroyTexture(old);
  }
  update_view(db);
  return SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                           SDL_TEXTUREACCESS_STREAMING,
                           view_w, view_h);
}

static void
render(SDL_Renderer* renderer, SDL_Texture* texture, Database* db, Cli* cli)
{
  Image* img = &db->img;
  uint8_t* packed = malloc(view_w * view_h * 3);
  memset(packed, 255, view_w * view_h * 3);

  write_text(packed, view_w, ">", 1, 1, 120, 120, 120);
  int text_x = 1 + FONT_WIDTH + 1 + 1;
  write_text(packed, view_w, cli->buf, text_x, 1, 0, 0, 0);

  int cursor_x = text_x + cli->cursor * (FONT_WIDTH + 1);
  if (cursor_x < view_w) {
    for (int cy = 0; cy < FONT_HEIGHT; cy++) {
      int idx = ((1 + cy) * view_w + cursor_x) * 3;
      packed[idx] = 0;
      packed[idx + 1] = 56;
      packed[idx + 2] = 255;
    }
  }

  for (int y = INPUT_ROW_H; y < view_h; y++) {
    int sy = y - INPUT_ROW_H + scroll_y;
    if (sy < 0 || sy >= img->height) {
      continue;
    }
    for (int x = 0; x < view_w && x < img->width; x++) {
      int src = (sy * img->alloc_width + x) * 3;
      int dst = (y * view_w + x) * 3;
      packed[dst] = img->data[src];
      packed[dst + 1] = img->data[src + 1];
      packed[dst + 2] = img->data[src + 2];
    }
  }

  if (show_grid) {
    int gx = FONT_WIDTH + 1;
    int gy = FONT_HEIGHT + 2;
    int max_x = view_w < img->width ? view_w : img->width;
    for (int y = INPUT_ROW_H; y < view_h; y++) {
      int ay = y - INPUT_ROW_H + scroll_y;
      if (ay < 0 || ay >= img->height) {
        continue;
      }
      for (int x = 0; x < max_x; x++) {
        if (x % gx == 0 || ay % gy == 0) {
          int idx = (y * view_w + x) * 3;
          packed[idx] = (packed[idx] + 200) / 2;
          packed[idx + 1] = (packed[idx + 1] + 220) / 2;
          packed[idx + 2] = (packed[idx + 2] + 255) / 2;
        }
      }
    }
  }

  SDL_UpdateTexture(texture, NULL, packed, view_w * 3);
  free(packed);

  SDL_RenderClear(renderer);
  SDL_Rect dst_rect = { 0, 0, view_w * SCALE, view_h * SCALE };
  SDL_RenderCopy(renderer, texture, NULL, &dst_rect);
  SDL_RenderPresent(renderer);
}

static Stack stack = { 0 };

static void
handle_submit(char* line, Database* db, bool* running)
{
  while (*line && isspace(*line)) {
    line++;
  }
  if (!*line) {
    return;
  }

  if (strcasecmp(line, "quit") == 0 || strcasecmp(line, "exit") == 0) {
    *running = false;
    return;
  }

  forth_eval(line, db, &stack);
  db_sync_stack(db, stack.items, stack.top);
}

int
main(int argc, char* argv[])
{
  Database db;
  db_init(&db);

  if (argc > 1) {
    if (db_load(&db, argv[1]) == 0) {
      printf("Loaded %s\n", argv[1]);
      stack.top = db_load_stack(&db, stack.items, MAX_STACK);
    }
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL: %s\n", SDL_GetError());
    return 1;
  }

  update_view(&db);

  SDL_Window* window =
    SDL_CreateWindow("fliptable",
                     SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                     view_w * SCALE, view_h * SCALE,
                     SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  SDL_Renderer* renderer =
    SDL_CreateRenderer(window, -1,
                       SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_Texture* texture =
    remake_texture(renderer, NULL, &db);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_StartTextInput();

  Cli cli;
  cli_init(&cli);

  bool running = true;
  char submitted[MAX_INPUT];

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;

      } else if (event.type == SDL_TEXTINPUT) {
        for (int i = 0; event.text.text[i]; i++) {
          cli_insert(&cli, event.text.text[i]);
        }

      } else if (event.type == SDL_MOUSEWHEEL) {
        scroll_y -= event.wheel.y * SCROLL_STEP;
        clamp_scroll(&db);

      } else if (event.type == SDL_KEYDOWN) {
        SDL_Keymod mod = SDL_GetModState();
        SDL_Keycode key = event.key.keysym.sym;

        if (key == SDLK_RETURN) {
          if (cli_submit(&cli, submitted, sizeof(submitted))) {
            handle_submit(submitted, &db, &running);
            texture = remake_texture(renderer, texture, &db);
            clamp_scroll(&db);
          }

        } else if (key == SDLK_BACKSPACE) {
          cli_backspace(&cli);
        } else if (key == SDLK_DELETE) {
          cli_delete(&cli);
        } else if (key == SDLK_LEFT) {
          cli_left(&cli);
        } else if (key == SDLK_RIGHT) {
          cli_right(&cli);
        } else if (key == SDLK_HOME ||
                   ((mod & KMOD_CTRL) && key == SDLK_a)) {
          cli_home(&cli);
        } else if (key == SDLK_END ||
                   ((mod & KMOD_CTRL) && key == SDLK_e)) {
          cli_end(&cli);
        } else if ((mod & KMOD_CTRL) && key == SDLK_u) {
          cli_clear(&cli);

        } else if (key == SDLK_UP) {
          if (mod & KMOD_CTRL) {
            scroll_y -= SCROLL_STEP;
            clamp_scroll(&db);
          } else {
            cli_history_up(&cli);
          }
        } else if (key == SDLK_DOWN) {
          if (mod & KMOD_CTRL) {
            scroll_y += SCROLL_STEP;
            clamp_scroll(&db);
          } else {
            cli_history_down(&cli);
          }

        } else if ((mod & KMOD_CTRL) && key == SDLK_MINUS) {
          if (db.img_scale > 0.1f) {
            db.img_scale *= 0.5f;
            db_render(&db);
            texture = remake_texture(renderer, texture, &db);
            clamp_scroll(&db);
          }
        } else if ((mod & KMOD_CTRL) &&
                   (key == SDLK_EQUALS || key == SDLK_PLUS)) {
          db.img_scale *= 2.0f;
          if (db.img_scale > 4.0f) {
            db.img_scale = 4.0f;
          }
          db_render(&db);
          texture = remake_texture(renderer, texture, &db);
          clamp_scroll(&db);
        } else if ((mod & KMOD_CTRL) && key == SDLK_g) {
          show_grid = !show_grid;
        }
      }
    }

    render(renderer, texture, &db, &cli);
  }

  SDL_StopTextInput();
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  db_free(&db);
  return 0;
}
