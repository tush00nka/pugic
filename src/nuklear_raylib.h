#ifndef NK_RAYLIB_H_
#define NK_RAYLIB_H_

#include "raylib.h"
#include <stdlib.h>

// Forward declarations
struct nk_context *nk_raylib_init(int width, int height);
void nk_raylib_input(struct nk_context *ctx);
void nk_raylib_render(struct nk_color clearColor, struct nk_color foreColor,
                      Font font);
void nk_raylib_shutdown(void);

#ifdef NK_RAYLIB_IMPLEMENTATION

static struct {
  struct nk_context ctx;
  struct nk_buffer cmds;
  struct nk_draw_null_texture null;
} raylib_state = {0};

struct nk_context *nk_raylib_init(int width, int height) {
  nk_buffer_init_default(&raylib_state.cmds);

  if (!nk_init_default(&raylib_state.ctx, NULL)) {
    return NULL;
  }

  return &raylib_state.ctx;
}

void nk_raylib_input(struct nk_context *ctx) {
  nk_input_begin(ctx);

  // Mouse input
  Vector2 mousePos = GetMousePosition();
  nk_input_motion(ctx, (int)mousePos.x, (int)mousePos.y);

  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)mousePos.x, (int)mousePos.y, 1);
  } else {
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)mousePos.x, (int)mousePos.y, 0);
  }

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)mousePos.x, (int)mousePos.y, 1);
  } else {
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)mousePos.x, (int)mousePos.y, 0);
  }

  // Mouse wheel
  nk_input_scroll(ctx, nk_vec2(0, GetMouseWheelMoveV().y * 5));

  // Keyboard input
  if (IsKeyPressed(KEY_ENTER)) {
    nk_input_key(ctx, NK_KEY_ENTER, 1);
  }
  if (IsKeyPressed(KEY_BACKSPACE)) {
    nk_input_key(ctx, NK_KEY_BACKSPACE, 1);
  }
  if (IsKeyPressed(KEY_DELETE)) {
    nk_input_key(ctx, NK_KEY_DEL, 1);
  }
  if (IsKeyPressed(KEY_TAB)) {
    nk_input_key(ctx, NK_KEY_TAB, 1);
  }
  if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
    nk_input_key(ctx, NK_KEY_CTRL, 1);
  }
  if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
    nk_input_key(ctx, NK_KEY_SHIFT, 1);
  }

  // Arrow keys
  if (IsKeyPressed(KEY_LEFT)) {
    nk_input_key(ctx, NK_KEY_LEFT, 1);
  }
  if (IsKeyPressed(KEY_RIGHT)) {
    nk_input_key(ctx, NK_KEY_RIGHT, 1);
  }
  if (IsKeyPressed(KEY_UP)) {
    nk_input_key(ctx, NK_KEY_UP, 1);
  }
  if (IsKeyPressed(KEY_DOWN)) {
    nk_input_key(ctx, NK_KEY_DOWN, 1);
  }

  // Character input
  int key = GetCharPressed();
  while (key > 0) {
    nk_input_unicode(ctx, (nk_rune)key);
    key = GetCharPressed();
  }

  nk_input_end(ctx);
}

void nk_raylib_render(struct nk_color clearColor, struct nk_color foreColor,
                      Font font) {
  struct nk_context *ctx = &raylib_state.ctx;

  const struct nk_command *cmd = NULL;
  nk_foreach(cmd, ctx) {
    if (!cmd)
      continue;

    switch (cmd->type) {
    case NK_COMMAND_NOP:
      break;
    case NK_COMMAND_SCISSOR: {
      const struct nk_command_scissor *s =
          (const struct nk_command_scissor *)cmd;
      EndScissorMode();
      if (s->w > 0 && s->h > 0) {
        BeginScissorMode(s->x, s->y, s->w, s->h);
      }
    } break;
    case NK_COMMAND_LINE: {
      const struct nk_command_line *l = (const struct nk_command_line *)cmd;
      DrawLine(l->begin.x, l->begin.y, l->end.x, l->end.y,
               (Color){l->color.r, l->color.g, l->color.b, l->color.a});
    } break;
    case NK_COMMAND_RECT: {
      const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
      DrawRectangleLines(
          r->x, r->y, r->w, r->h,
          (Color){r->color.r, r->color.g, r->color.b, r->color.a});
    } break;
    case NK_COMMAND_RECT_FILLED: {
      const struct nk_command_rect_filled *r =
          (const struct nk_command_rect_filled *)cmd;
      DrawRectangle(r->x, r->y, r->w, r->h,
                    (Color){r->color.r, r->color.g, r->color.b, r->color.a});
    } break;
    case NK_COMMAND_CIRCLE: {
      const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
      DrawCircleLines(c->x + c->w / 2, c->y + c->h / 2, c->w / 2,
                      (Color){c->color.r, c->color.g, c->color.b, c->color.a});
    } break;
    case NK_COMMAND_CIRCLE_FILLED: {
      const struct nk_command_circle_filled *c =
          (const struct nk_command_circle_filled *)cmd;
      DrawCircle(c->x + c->w / 2, c->y + c->h / 2, c->w / 2,
                 (Color){c->color.r, c->color.g, c->color.b, c->color.a});
    } break;
    case NK_COMMAND_TEXT: {
      const struct nk_command_text *t = (const struct nk_command_text *)cmd;
      if (t->length > 0) {
        DrawTextEx(font, (const char *)t->string, (Vector2){t->x, t->y},
                   (float)t->height, 0.0f,
                   (Color){t->foreground.r, t->foreground.g, t->foreground.b,
                           t->foreground.a});
      }
    } break;
    case NK_COMMAND_IMAGE: { // <-- FIXED: Added "case" keyword
      const struct nk_command_image *img_cmd =
          (const struct nk_command_image *)cmd;
      Texture *tex = (Texture *)img_cmd->img.handle.ptr;
      if (tex && tex->id != 0) {
        Rectangle source = {img_cmd->img.region[0], img_cmd->img.region[1],
                            img_cmd->img.region[2], img_cmd->img.region[3]};
        Rectangle dest = {img_cmd->x, img_cmd->y, img_cmd->w, img_cmd->h};
        DrawTexturePro(*tex, source, dest, (Vector2){0, 0}, 0, WHITE);
      }
    } break;
    case NK_COMMAND_POLYGON:
    case NK_COMMAND_POLYGON_FILLED:
    case NK_COMMAND_POLYLINE:
    case NK_COMMAND_CURVE:
      // Not implemented yet
      break;
    default:
      break;
    }
  }

  EndScissorMode();
  nk_clear(ctx);
}

void nk_raylib_shutdown(void) {
  nk_buffer_free(&raylib_state.cmds);
  nk_free(&raylib_state.ctx);
}

#endif // NK_RAYLIB_IMPLEMENTATION
#endif // NK_RAYLIB_H_
