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

void JSON_InitPrinter(jsonPrinter_t *p, jsonPrinterPrintChar_f printChar, jsonPrinterPrintChars_f printChars)
{
	p->PrintChar = printChar;
	p->PrintChars = printChars;
	p->lastChar = 0;
}

static void JSON_PrintChar(jsonPrinter_t *p, char ch)
{
	p->PrintChar(ch);
	p->lastChar = ch;
}

static void JSON_PrintChars(jsonPrinter_t *p, const char *chars, int len)
{
	p->PrintChars(chars, len);
	if (len > 0) {
		p->lastChar = chars[len - 1];
	}
}

void JSON_PrintObjectStart(jsonPrinter_t *p)
{
	if (p->lastChar && p->lastChar != '[' && p->lastChar != ':') {
		JSON_PrintChar(p, ',');
	}

	JSON_PrintChar(p, '{');
}

void JSON_PrintObjectEnd(jsonPrinter_t *p)
{
	JSON_PrintChar(p, '}');
}

void JSON_PrintArrayStart(jsonPrinter_t *p)
{
	if (p->lastChar && p->lastChar != '[' && p->lastChar != ':') {
		JSON_PrintChar(p, ',');
	}

	JSON_PrintChar(p, '[');
}

void JSON_PrintArrayEnd(jsonPrinter_t *p)
{
	JSON_PrintChar(p, ']');
}

void JSON_PrintKey(jsonPrinter_t *p, const char *key)
{
	if (p->lastChar && p->lastChar != '{') {
		JSON_PrintChar(p, ',');
	}

	int keylen = strlen(key);

	JSON_PrintChar(p, '"');
	JSON_PrintChars(p, key, keylen);
	JSON_PrintChar(p, '"');
	JSON_PrintChar(p, ':');
}

static void JSON_PrintValue(jsonPrinter_t *p, const char *value)
{
	if (p->lastChar && p->lastChar != '[' && p->lastChar != ':') {
		JSON_PrintChar(p, ',');
	}

	int len = strlen(value);

	JSON_PrintChars(p, value, len);
}

void JSON_PrintBool(jsonPrinter_t *p, qboolean value)
{
	JSON_PrintValue(p, value ? "true" : "false");
}

void JSON_PrintCString(jsonPrinter_t *p, const char *string)
{
	if (p->lastChar && p->lastChar != '[' && p->lastChar != ':') {
		JSON_PrintChar(p, ',');
	}

	JSON_PrintChar(p, '"');

	for (const char *in = string; *in != '\0'; in++) {
		char ch = *in;
		if (ch < 0x20) {
			char hex[6];
			sprintf(hex, "u%.4x", ch);
			JSON_PrintChars(p, hex, 5);
		} else if (ch == '"' || ch == '\\') {
			JSON_PrintChar(p, '"');
			JSON_PrintChar(p, '\\');
		} else {
			JSON_PrintChar(p, ch);
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

//
// Buffered stream
//

static void JSON_BufferedStreamFlush(jsonBufferedStream_t *s)
{
	s->PrintChars(s->buf, s->cursor - s->buf);
	s->cursor = s->buf;
}


void JSON_BufferedStreamInit(jsonBufferedStream_t *s, char *buf, int buflen, jsonPrinterPrintChars_f printChars)
{
	s->PrintChars = printChars;
	s->buf = buf;
	s->bufend = buf + buflen;
	s->cursor = buf;
}

void JSON_BufferedStreamClose(jsonBufferedStream_t *s)
{
	JSON_BufferedStreamFlush(s);

	s->PrintChars = NULL;
	s->buf = 0;
	s->bufend = 0;
	s->cursor = 0;
}

void JSON_BufferedStreamPutChar(jsonBufferedStream_t *s, char ch)
{
	if (s->cursor >= s->bufend) {
		JSON_BufferedStreamFlush(s);
	}

	s->cursor[0] = ch;
	s->cursor++;
}

void JSON_BufferedStreamPutChars(jsonBufferedStream_t *s, const char *chars, int size)
{
	while (size > 0) {
		qboolean flush;
		int copySize = size;

		if (s->cursor + copySize > s->bufend) {
			copySize = (int)(s->bufend - s->cursor);
			flush = qtrue;
		} else {
			flush = qfalse;
		}

		memcpy(s->cursor, chars, copySize);

		s->cursor += copySize;
		chars += copySize;
		size -= copySize;

		if (flush) {
			JSON_BufferedStreamFlush(s);
		}
	}
}
