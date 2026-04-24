#include "db.h"
#include "glyph.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Cell*
find_cell(Database* db, char* key)
{
  return db_get_cell(db, key);
}

static void
assert_cell(Database* db, char* key, char* val, ValType type)
{
  Cell* c = find_cell(db, key);
  assert(c && "cell not found");
  assert(c->type == type);
  assert(strcmp(c->value, val) == 0);
}

static void
assert_no_cell(Database* db, char* key)
{
  Cell* c = find_cell(db, key);
  assert(!c && "cell should not exist");
}

static void
roundtrip(Database* db, char* file)
{
  db_save(db, file);
  db_free(db);
  db_init(db);
  int rc = db_load(db, file);
  assert(rc == 0 && "db_load failed");
}

int main(void)
{
  Database* db = calloc(1, sizeof(Database));
  int pass = 0;

  /* Test 1: single char namespace */
  db_init(db);
  db_set(db, "i.val", "test");
  roundtrip(db, "test1.png");
  assert(db->row_count == 1);
  assert_cell(db, "i.val", "test", VAL_TEXT);
  db_free(db);
  printf("  pass %d: single char namespace\n", ++pass);

  /* Test 2: many cells in one row */
  db_init(db);
  db_set(db, "a", "1");
  db_set(db, "b", "2");
  db_set(db, "c", "3");
  db_set(db, "d", "4");
  db_set(db, "e", "5");
  roundtrip(db, "test2.png");
  assert(db->row_count == 1);
  assert_cell(db, "a", "1", VAL_NUM);
  assert_cell(db, "b", "2", VAL_NUM);
  assert_cell(db, "c", "3", VAL_NUM);
  assert_cell(db, "d", "4", VAL_NUM);
  assert_cell(db, "e", "5", VAL_NUM);
  db_free(db);
  printf("  pass %d: many cells\n", ++pass);

  /* Test 3: deletion survives roundtrip */
  db_init(db);
  db_set(db, "a", "1");
  db_set(db, "b", "2");
  db_set(db, "c", "3");
  db_del(db, "b");
  roundtrip(db, "test3.png");
  assert_cell(db, "a", "1", VAL_NUM);
  assert_no_cell(db, "b");
  assert_cell(db, "c", "3", VAL_NUM);
  db_free(db);
  printf("  pass %d: deletion\n", ++pass);

  /* Test 4: longer values */
  db_init(db);
  db_set(db, "msg", "hello world foo bar");
  db_set(db, "num", "999999");
  roundtrip(db, "test4.png");
  assert_cell(db, "msg", "hello world foo bar", VAL_TEXT);
  assert_cell(db, "num", "999999", VAL_NUM);
  db_free(db);
  printf("  pass %d: long values\n", ++pass);

  /* Test 5: image cell */
  db_init(db);
  db_set(db, "title", "test");

  Cell* imgcell = calloc(1, sizeof(Cell));
  imgcell->type = VAL_IMAGE;
  imgcell->img_width = 8;
  imgcell->img_height = 8;
  imgcell->img_data = calloc(8 * 8 * 3, 1);
  for (int i = 0; i < 8*8*3; i++) imgcell->img_data[i] = (i % 3 == 0) ? 0 : 128;
  snprintf(imgcell->value, MAX_KEY, "test.png");
  db_set_cell(db, "pic", imgcell);
  cell_free_temp(imgcell);

  roundtrip(db, "test5.png");
  assert_cell(db, "title", "test", VAL_TEXT);
  Cell* pic = find_cell(db, "pic");
  assert(pic && pic->type == VAL_IMAGE);
  assert(strcmp(pic->value, "test.png") == 0);
  assert(pic->img_width == 8 && pic->img_height == 8);
  db_free(db);
  printf("  pass %d: image cell\n", ++pass);

  /* Test 6: deeply nested namespace */
  db_init(db);
  db_set(db, "a.b.c.d", "deep");
  roundtrip(db, "test6.png");
  assert(db->row_count == 1);
  assert_cell(db, "a.b.c.d", "deep", VAL_TEXT);
  db_free(db);
  printf("  pass %d: deep namespace\n", ++pass);

  /* Test 7: multiple namespaces with images */
  db_init(db);
  db_set(db, "a", "1");

  imgcell = calloc(1, sizeof(Cell));
  imgcell->type = VAL_IMAGE;
  imgcell->img_width = 20;
  imgcell->img_height = 20;
  imgcell->img_data = calloc(20 * 20 * 3, 1);
  for (int i = 0; i < 20*20*3; i++) imgcell->img_data[i] = (i % 3 == 0) ? 0 : 128;
  snprintf(imgcell->value, MAX_KEY, "img.png");
  db_set_cell(db, "images.test", imgcell);
  cell_free_temp(imgcell);

  db_set(db, "other.x", "hello");
  roundtrip(db, "test7.png");
  assert(db->row_count == 3);
  assert_cell(db, "a", "1", VAL_NUM);
  Cell* img = find_cell(db, "images.test");
  assert(img && img->type == VAL_IMAGE);
  assert(img->img_width == 20 && img->img_height == 20);
  assert_cell(db, "other.x", "hello", VAL_TEXT);
  db_free(db);
  printf("  pass %d: multiple namespaces with images\n", ++pass);

  free(db);
  printf("\nall %d tests passed\n", pass);
  return 0;
}
