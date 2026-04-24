#include "cli.h"

#include <stdio.h>
#include <string.h>

void
cli_init(Cli* cli)
{
  memset(cli, 0, sizeof(Cli));
}

void
cli_insert(Cli* cli, char ch)
{
  if (cli->len >= MAX_INPUT - 1) {
    return;
  }
  memmove(cli->buf + cli->cursor + 1, cli->buf + cli->cursor,
          cli->len - cli->cursor + 1);
  cli->buf[cli->cursor] = ch;
  cli->len++;
  cli->cursor++;
}

void
cli_backspace(Cli* cli)
{
  if (cli->cursor > 0) {
    memmove(cli->buf + cli->cursor - 1, cli->buf + cli->cursor,
            cli->len - cli->cursor + 1);
    cli->cursor--;
    cli->len--;
  }
}

void
cli_delete(Cli* cli)
{
  if (cli->cursor < cli->len) {
    memmove(cli->buf + cli->cursor, cli->buf + cli->cursor + 1,
            cli->len - cli->cursor);
    cli->len--;
  }
}

void
cli_clear(Cli* cli)
{
  cli->buf[0] = '\0';
  cli->len = 0;
  cli->cursor = 0;
}

void
cli_left(Cli* cli)
{
  if (cli->cursor > 0) {
    cli->cursor--;
  }
}

void
cli_right(Cli* cli)
{
  if (cli->cursor < cli->len) {
    cli->cursor++;
  }
}

void
cli_home(Cli* cli)
{
  cli->cursor = 0;
}

void
cli_end(Cli* cli)
{
  cli->cursor = cli->len;
}

void
cli_history_up(Cli* cli)
{
  if (cli->history_pos > 0) {
    cli->history_pos--;
    strncpy(cli->buf, cli->history[cli->history_pos], MAX_INPUT - 1);
    cli->len = strlen(cli->buf);
    cli->cursor = cli->len;
  }
}

void
cli_history_down(Cli* cli)
{
  if (cli->history_pos < cli->history_count - 1) {
    cli->history_pos++;
    strncpy(cli->buf, cli->history[cli->history_pos], MAX_INPUT - 1);
    cli->len = strlen(cli->buf);
    cli->cursor = cli->len;
  } else if (cli->history_pos < cli->history_count) {
    cli->history_pos = cli->history_count;
    cli_clear(cli);
  }
}

bool
cli_submit(Cli* cli, char* out, int outsize)
{
  if (cli->len == 0) {
    return false;
  }
  snprintf(out, outsize, "%s", cli->buf);

  char tmp[MAX_INPUT];
  memcpy(tmp, cli->buf, MAX_INPUT);

  if (cli->history_count < MAX_HISTORY) {
    memcpy(cli->history[cli->history_count], tmp, MAX_INPUT);
    cli->history_count++;
  } else {
    for (int i = 0; i < MAX_HISTORY - 1; i++) {
      memcpy(cli->history[i], cli->history[i + 1], MAX_INPUT);
    }
    memcpy(cli->history[MAX_HISTORY - 1], tmp, MAX_INPUT);
  }
  cli->history_pos = cli->history_count;

  cli_clear(cli);
  return true;
}

void
cli_set(Cli* cli, char* text)
{
  strncpy(cli->buf, text, MAX_INPUT - 1);
  cli->buf[MAX_INPUT - 1] = '\0';
  cli->len = strlen(cli->buf);
  cli->cursor = cli->len;
}
