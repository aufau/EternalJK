/*
===========================================================================
Copyright (C) 2025, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "qcommon/qcommon.h"
#include "json.h"

// json.cpp  -- JSON printer

void JSON_InitPrinter(jsonPrinter_t *p, char *buf, int buflen)
{
	p->buf = buf;
	p->cursor = buf;
	p->bufend = buf + buflen;
	p->overflow = false;
}

int JSON_PrintedLength(jsonPrinter_t *p)
{
	return (int)(p->cursor - p->buf);
}

static void JSON_PrintChar(jsonPrinter_t *p, char ch)
{
	if (p->cursor >= p->bufend) {
		p->overflow = true;
		return;
	}

	p->cursor[0] = ch;
	p->cursor++;
}

void JSON_PrintObjectStart(jsonPrinter_t *p)
{
	JSON_PrintChar(p, '{');
}

void JSON_PrintObjectEnd(jsonPrinter_t *p)
{
	JSON_PrintChar(p, '}');
}

void JSON_PrintArrayStart(jsonPrinter_t *p)
{
	JSON_PrintChar(p, '[');
}

void JSON_PrintArrayEnd(jsonPrinter_t *p)
{
	JSON_PrintChar(p, ']');
}

void JSON_PrintKey(jsonPrinter_t *p, const char *key)
{
	if (p->cursor > p->buf) {
		if (p->cursor[-1] != '{') {
			if (p->cursor >= p->bufend) {
				p->overflow = true;
				return;
			}

			p->cursor[0] = ',';
			p->cursor++;
		}
	}

	int keylen = strlen(key);

	if (p->cursor + keylen + 3 >= p->bufend) {
		p->overflow = true;
		return;
	}

	p->cursor[0] = '"';
	memcpy(p->cursor + 1, key, keylen);
	p->cursor[keylen + 1] = '"';
	p->cursor[keylen + 2] = ':';
	p->cursor += keylen + 3;
}

static void JSON_PrintValue(jsonPrinter_t *p, const char *value)
{
	if (p->cursor > p->buf) {
		if (p->cursor[-1] != '[' && p->cursor[-1] != ':') {
			if (p->cursor >= p->bufend) {
				p->overflow = true;
				return;
			}

			p->cursor[0] = ',';
			p->cursor++;
		}
	}

	int len = strlen(value);

	if (p->cursor + len >= p->bufend) {
		p->overflow = true;
		return;
	}

	memcpy(p->cursor, value, len);
	p->cursor += len;
}

void JSON_PrintBool(jsonPrinter_t *p, qboolean value)
{
	JSON_PrintValue(p, value ? "true" : "false");
}

void JSON_PrintCString(jsonPrinter_t *p, const char *string)
{
	if (p->overflow) {
		return;
	}

	if (p->cursor > p->buf) {
		if (p->cursor[-1] != '[' && p->cursor[-1] != ':') {
			if (p->cursor >= p->bufend) {
				p->overflow = true;
				return;
			}

			p->cursor[0] = ',';
			p->cursor++;
		}
	}

	JSON_PrintChar(p, '"');

	for (const char *in = string; *in != '\0'; in++) {
		char ch = *in;
		if (ch < 0x20) {
			if (p->cursor + 5 >= p->bufend) {
				p->overflow = true;
				return;
			}

			sprintf(p->cursor, "u%.4x", ch);
			p->cursor += 5;
		} else if (ch == '"' || ch == '\\') {
			if (p->cursor + 2 >= p->bufend) {
				p->overflow = true;
				return;
			}

			p->cursor[0] = '\\';
			p->cursor[1] = ch;
			p->cursor += 2;
		} else {
			if (p->cursor + 1 >= p->bufend) {
				p->overflow = true;
				return;
			}

			p->cursor[0] = ch;
			p->cursor++;
		}
	}

	JSON_PrintChar(p, '"');
}

void JSON_PrintInt32(jsonPrinter_t *p, int32_t i)
{
	char number[12]; // -2147483648
	snprintf(number, sizeof(number), "%d", i);
	JSON_PrintValue(p, number);
}

void JSON_PrintByte(jsonPrinter_t *p, byte b)
{
	char number[4]; // 255
	snprintf(number, sizeof(number), "%d", b);
	JSON_PrintValue(p, number);
}

void JSON_PrintFloat(jsonPrinter_t *p, float f)
{
	char number[20]; // -1.0000000000e-126
	snprintf(number, sizeof(number), "%.10e", f);
	JSON_PrintValue(p, number);
}
