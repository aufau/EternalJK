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

#pragma once

// json.h -- JSON printer API

typedef struct jsonPrinter_s {
	char *buf;
	char *bufend;
	char *cursor;
	bool overflow;
} jsonPrinter_t;

void JSON_InitPrinter(jsonPrinter_t *p, char *buf, int buflen);
int JSON_PrintedLength(jsonPrinter_t *p);
void JSON_PrintObjectStart(jsonPrinter_t *p);
void JSON_PrintObjectEnd(jsonPrinter_t *p);
void JSON_PrintArrayStart(jsonPrinter_t *p);
void JSON_PrintArrayEnd(jsonPrinter_t *p);
void JSON_PrintKey(jsonPrinter_t *p, const char *key);
void JSON_PrintBool(jsonPrinter_t *p, qboolean value);
void JSON_PrintCString(jsonPrinter_t *p, const char *string);
void JSON_PrintInt32(jsonPrinter_t *p, int32_t i);
void JSON_PrintByte(jsonPrinter_t *p, byte b);
void JSON_PrintFloat(jsonPrinter_t *p, float f);
