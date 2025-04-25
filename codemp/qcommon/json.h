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

typedef void (*jsonPrinterPrintChar_f)(char);
typedef void (*jsonPrinterPrintChars_f)(const char *, int len);

typedef struct jsonPrinter_s {
	jsonPrinterPrintChar_f PrintChar;
	jsonPrinterPrintChars_f PrintChars;
	char lastChar;
} jsonPrinter_t;

void JSON_InitPrinter(jsonPrinter_t *p, jsonPrinterPrintChar_f printChar, jsonPrinterPrintChars_f printChars);
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

//
// Buffered file stream
//

typedef struct jsonFileStream_s {
	fileHandle_t fileHandle;
	char *buf;
	char *bufend;
	char *cursor;
} jsonFileStream_t;

void JSON_FileStreamInit(jsonFileStream_t *s, fileHandle_t fh, char *buf, int buflen);
void JSON_FileStreamClose(jsonFileStream_t *s);
void JSON_FileStreamPutChar(jsonFileStream_t *s, char ch);
void JSON_FileStreamPutChars(jsonFileStream_t *s, const char *chars, int size);
