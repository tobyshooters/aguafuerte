#include "db.h"
#include "glyph.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
  Database* db = calloc(1, sizeof(Database));
  db_init(db);

  Cell* imgcell = calloc(1, sizeof(Cell));
  imgcell->type = VAL_IMAGE;
  imgcell->img_width = 20;
  imgcell->img_height = 20;
  imgcell->img_data = malloc(20 * 20 * 3);
  for (int i = 0; i < 20*20*3; i++) imgcell->img_data[i] = (i % 3 == 0) ? 0 : 128;
  snprintf(imgcell->value, MAX_KEY, "test.png");

  db_set_cell(db, "images.test", imgcell);
  db_set(db, "other", "hello");
  cell_free_temp(imgcell);

  assert(db->row_count == 2);

  db_save(db, "test_img_ns.png");
  db_free(db);

  db_init(db);
  int rc = db_load(db, "test_img_ns.png");
  assert(rc == 0);
  assert(db->row_count == 2);

  Cell* img = db_get_cell(db, "images.test");
  assert(img);
  assert(img->type == VAL_IMAGE);
  assert(strcmp(img->value, "test.png") == 0);
  assert(img->img_width == 20 && img->img_height == 20);

  Cell* other = db_get_cell(db, "other");
  assert(other);
  assert(strcmp(other->value, "hello") == 0);

  db_free(db);
  free(db);
  printf("  pass: image namespace roundtrip\n");
  return 0;
}
