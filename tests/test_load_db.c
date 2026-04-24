#include "db.h"
#include "glyph.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(void)
{
  int w, h, ch;
  uint8_t* raw = stbi_load("tests/test_db", &w, &h, &ch, 3);
  assert(raw && "can't load tests/test_db");
  assert(w == 163 && h == 197);

  assert(read_text(raw, w, h, 1, 1) != NULL);
  assert(strcmp(read_text(raw, w, h, 1, 1), "stack") == 0);

  assert(read_text(raw, w, h, 1, 25) != NULL);
  assert(strcmp(read_text(raw, w, h, 1, 25), "home") == 0);

  assert(read_text(raw, w, h, 1, 129) != NULL);
  assert(strcmp(read_text(raw, w, h, 1, 129), "images") == 0);

  stbi_image_free(raw);
  printf("  pass: load test_db\n");
  return 0;
}
