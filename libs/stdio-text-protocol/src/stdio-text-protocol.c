#include "stdio-text-protocol/stdio-text-protocol.h"
#include <stddef.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "logging/log.h"

static char line[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint line_length = 0;
static command_t command;



void stdio_text_protocol_init(void) {}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);
    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    if (symbol == '\r' || symbol == '\n')
    {       
        putchar('\n');
        line[line_length] = '\0';

        if (line_length > 0)
        {
            LOG_DBG("got %s\n", line);

            bool got_name = false;
            command.truncated = false;
            command.argc = 0;
            for (uint i = 0; i < line_length; ++i)
            {
                if (line[i] == ' ')
                {
                    line[i] = '\0';
                }
                if ((i > 0 && line[i] != '\0' && line[i - 1] == '\0') || (i == 0 && line[i] != '\0'))
                {
                    if (!got_name)
                    {
                        command.name = line + i;
                        got_name = true;
                        continue;
                    }
                    if (command.argc + 1 > STDIO_TEXT_PROTOCOL_MAX_ARGS)
                    {
                        command.truncated = true;
                        line_length = 0;
                        return &command;
                    }
                    command.argv[command.argc] = line + i;
                    command.argc += 1;
                }
            }
            line_length = 0;
            return &command;
        }
        return NULL;
    }

    if ((symbol == 0x80 || symbol == 0x7f) && line_length > 0)
    {
        line_length -= 1;
        line[line_length] = '\0';
        putchar('\b');
        putchar(' ');
        return NULL;
    }

    if (line_length + 1 < STDIO_TEXT_PROTOCOL_LINE_SIZE)
    {
        line[line_length] = (char)symbol;
        line_length += 1;
        putchar(symbol);
        return NULL;
    }
    else
    {
        command.truncated = true;
        line_length = 0;
        return &command;
    }
}