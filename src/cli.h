#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#define MAX_INPUT 512
#define MAX_HISTORY 64

typedef struct
{
  char buf[MAX_INPUT];
  int len;
  int cursor;
  char history[MAX_HISTORY][MAX_INPUT];
  int history_count;
  int history_pos;
} Cli;

void
cli_init(Cli* cli);
void
cli_insert(Cli* cli, char ch);
void
cli_backspace(Cli* cli);
void
cli_delete(Cli* cli);
void
cli_clear(Cli* cli);
void
cli_left(Cli* cli);
void
cli_right(Cli* cli);
void
cli_home(Cli* cli);
void
cli_end(Cli* cli);
void
cli_history_up(Cli* cli);
void
cli_history_down(Cli* cli);
bool
cli_submit(Cli* cli, char* out, int outsize);

#endif
