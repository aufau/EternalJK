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

// cl_ai.c  -- training and interfacing with AI model acting as player

#include "client.h"
#include "qcommon/json.h"

extern netField_t playerStateFields[];
extern int playerStateFieldsNum;

extern netField_t entityStateFields[];
extern int entityStateFieldsNum;

typedef void (*aiEventHandler_f)(const char *);

typedef struct aiEventStream_s {
	char *event;
	int eventSize;
	int eventCursor;
	qboolean writeOverflow;
	aiEventHandler_f EventHandler;
} aiEventStream_t;

#define AI_MAX_EVENT_SIZE 64

struct aiStatic_s {
	jsonPrinter_t	jp;
	jsonBufferedStream_t jpbs;
	char evBuf[AI_MAX_EVENT_SIZE + 1]; // extra byte for \0
	aiEventStream_t evStream;
	netadr_t udpAgentAdr;
};

struct aiStatic_s ais;


//
// newline-delimited event stream for TCP
//

void AI_InitEventStream( aiEventStream_t *s, char *buf, int bufSize, aiEventHandler_f eventHandler ) {
	s->event = buf;
	s->eventSize = bufSize - 1; // save extra byte for \0
	s->eventCursor = 0;
	s->writeOverflow = qfalse;
	s->EventHandler = eventHandler;
}

void AI_CloseEventStream( aiEventStream_t *s ) {
	s->event = NULL;
}

qboolean AI_IsEventStreamOpen( aiEventStream_t *s ) {
	return (qboolean)(s->event != NULL);
}

void AI_EventStreamWrite( aiEventStream_t *s, const char *data, int dataSize ) {
	int dataCursor;

	for (dataCursor = 0; dataCursor < dataSize; dataCursor++)
	{
		char ch = data[dataCursor];

		if (s->eventCursor == s->eventSize) {
			s->writeOverflow = qtrue;
		}

		if (s->writeOverflow) {
			if (ch == '\n') {
				Com_Printf("WARNING AI_EventStreamWrite: ignored oversize message\n");
				s->writeOverflow = qfalse;
				s->eventCursor = 0;
			}
		} else {
			if (ch == '\n') {
				s->event[s->eventCursor + 1] = '\0';
				s->EventHandler(s->event);
				s->eventCursor = 0;
			} else {
				s->event[s->eventCursor++] = ch;
			}
		}
	}
}

//
// JSON Stream
//

static void AI_JP_PrintChar(char ch)
{
	JSON_BufferedStreamPutChar(&ais.jpbs, ch);
}

static void AI_JP_PrintChars(const char *chars, int len)
{
	JSON_BufferedStreamPutChars(&ais.jpbs, chars, len);
}

static void AI_JP_PrintCharsFile(const char *chars, int len)
{
	FS_Write(chars, len, com_playerPerspectiveF);
}

static void AI_JP_InitFile(char *buf, int buflen)
{
	JSON_BufferedStreamInit(&ais.jpbs, buf, buflen, AI_JP_PrintCharsFile);
	JSON_InitPrinter(&ais.jp, AI_JP_PrintChar, AI_JP_PrintChars);
}

static void AI_JP_CloseFile()
{
	JSON_BufferedStreamClose(&ais.jpbs);
}

static void AI_JP_ObjectStart(void)
{
	JSON_PrintObjectStart(&ais.jp);
}

static void AI_JP_ObjectEnd(void)
{
	JSON_PrintObjectEnd(&ais.jp);
}

static void AI_JP_ArrayStart(void)
{
	JSON_PrintArrayStart(&ais.jp);
}

static void AI_JP_ArrayEnd(void)
{
	JSON_PrintArrayEnd(&ais.jp);
}

static void AI_JP_Key(const char *key)
{
	JSON_PrintKey(&ais.jp, key);
}

static void AI_JP_Int32(int i)
{
	JSON_PrintInt32(&ais.jp, i);
}

static void AI_JP_Byte(byte b)
{
	JSON_PrintByte(&ais.jp, b);
}

static void AI_JP_Bool(qboolean value)
{
	JSON_PrintBool(&ais.jp, value);
}

static void AI_JP_Float(float f)
{
	JSON_PrintFloat(&ais.jp, f);
}

static void AI_JP_PlayerState(const playerState_t *ps)
{
	AI_JP_ObjectStart();
	{
		const netField_t *PSFields = playerStateFields;
		const netField_t *field;
		int numFields = playerStateFieldsNum;

		for (field = PSFields; field < PSFields + numFields; field++) {
			void *fp = (byte *)ps + field->offset;

			AI_JP_Key(field->name);

			if (field->bits == 0) {
				// float
				AI_JP_Float(*(float *)fp);
			} else if (field->bits == 1) {
				// qboolean
				AI_JP_Bool(*(qboolean *)fp);
			} else {
				// integer
				AI_JP_Int32(*(int *)fp);
			}
		}

		AI_JP_Key("stats");
		AI_JP_ArrayStart();
		{
			for (int i = 0; i < MAX_STATS; i++) {
				AI_JP_Int32(ps->stats[i]);
			}
		}
		AI_JP_ArrayEnd();

		AI_JP_Key("persistant");
		AI_JP_ArrayStart();
		{
			for (int i = 0; i < MAX_PERSISTANT; i++) {
				AI_JP_Int32(ps->persistant[i]);
			}
		}
		AI_JP_ArrayEnd();

		AI_JP_Key("powerups");
		AI_JP_ArrayStart();
		{
			for (int i = 0; i < MAX_POWERUPS; i++) {
				AI_JP_Int32(ps->powerups[i]);
			}
		}
		AI_JP_ArrayEnd();

		AI_JP_Key("ammo");
		AI_JP_ArrayStart();
		{
			for (int i = 0; i < MAX_AMMO; i++) {
				AI_JP_Int32(ps->ammo[i]);
			}
		}
		AI_JP_ArrayEnd();
	}
	AI_JP_ObjectEnd();
}

static void AI_JP_EntityState(const entityState_t *es)
{
	AI_JP_ObjectStart();
	{
		const netField_t *ESFields = entityStateFields;
		const netField_t *field;
		int numFields = entityStateFieldsNum;

		AI_JP_Key("number"); AI_JP_Int32(es->number);

		for (field = ESFields; field < ESFields + numFields; field++) {
			void *fp = (byte *)es + field->offset;

			AI_JP_Key(field->name);

			if (field->bits == 0) {
				// float
				AI_JP_Float(*(float *)fp);
			} else if (field->bits == 1) {
				// qboolean
				AI_JP_Bool(*(qboolean *)fp);
			} else {
				// integer
				AI_JP_Int32(*(int *)fp);
			}
		}

	}
	AI_JP_ObjectEnd();
}

void AI_RecordClientSnapshot(const clSnapshot_t *snap)
{
	char buf[4096];

	AI_JP_InitFile(buf, sizeof(buf));

	AI_JP_ObjectStart();
	{
		AI_JP_Key("frame"); AI_JP_Int32(com_frameNumber);
		// AI_JP_Key("valid"); AI_JP_Bool(snap->valid);
		// AI_JP_Key("snapFlags"); AI_JP_Int32(snap->snapFlags);
		AI_JP_Key("serverTime"); AI_JP_Int32(snap->serverTime);
		AI_JP_Key("messageNum"); AI_JP_Int32(snap->messageNum);
		AI_JP_Key("deltaNum"); AI_JP_Int32(snap->deltaNum);
		AI_JP_Key("ping"); AI_JP_Int32(snap->ping);
		AI_JP_Key("areamask"); AI_JP_ArrayStart(); {
			for (int i = 0; i < MAX_MAP_AREA_BYTES; i++) {
				AI_JP_Byte(snap->areamask[i]);
			}
		}; AI_JP_ArrayEnd();
		AI_JP_Key("cmdNum"); AI_JP_Int32(snap->cmdNum);
		AI_JP_Key("ps"); AI_JP_PlayerState(&snap->ps);
		AI_JP_Key("numEntities"); AI_JP_Int32(snap->numEntities);
		AI_JP_Key("entities"); AI_JP_ArrayStart();
		{
			for (int i = 0; i < snap->numEntities; i++) {
				int entNum = (snap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1);
				AI_JP_EntityState(&cl.parseEntities[entNum]);
			}
		}; AI_JP_ArrayEnd();
		AI_JP_Key("serverCommandNum"); AI_JP_Int32(snap->serverCommandNum);
	}
	AI_JP_ObjectEnd();

	AI_JP_PrintChar('\n');
	AI_JP_CloseFile();
}

void AI_RecordSysEvent(const sysEvent_t *ev)
{
	if (!com_actionDataF) {
		return;
	}

	const char* eventType = Sys_EventName(ev->evType);
	const char* json;

	switch (ev->evType) {
	case SE_KEY: {
		const char* keyName = Key_KeynumToString(ev->evValue);
		json = va("{\"time\":%d,\"frame\":%d,\"type\":\"%s\",\"code\":%d,\"name\":\"%s\",\"down\":%s}\n", ev->evTime, com_frameNumber, eventType, ev->evValue, keyName, ev->evValue2 ? "true" : "false");
		break;
	}
	case SE_CHAR: {
		const char* charName;

		if ('a' - 'a' + 1 <= ev->evValue && ev->evValue <= 'z' - 'a' + 1) {
			charName = va("Ctrl+%c", ev->evValue + 'a' - 1);
		}
		else if (0x20 < ev->evValue && ev->evValue < 0x7f) {
			charName = va("%c", ev->evValue);
		}
		else {
			charName = "<?>"; // extended ascii, other control codes
		}
		json = va("{\"time\":%d,\"frame\":%d,\"type\":\"%s\",\"code\":%d,\"name\":\"%s\"}\n", ev->evTime, com_frameNumber, eventType, ev->evValue, charName);
		break;
	}
	case SE_MOUSE: {
		json = va("{\"time\":%d,\"frame\":%d,\"type\":\"%s\",\"dx\":%d,\"dy\":%d}\n", ev->evTime, com_frameNumber, eventType, ev->evValue, ev->evValue2);
		break;
	}
	case SE_NONE:
		return;
	case SE_CONSOLE:
	default: {
		json = va("{\"time\":%d,\"frame\":%d,\"type\":\"%s\"}\n", ev->evTime, com_frameNumber, eventType);
		break;
	}
	}

	FS_Write(json, strlen(json), com_actionDataF);
}

void AI_AgentMessage( const char *msg )
{
	int type, value, value2;
	int ret;

	Com_DPrintf("AI_AgentMessage: %s", msg);
	ret = sscanf(msg, "%d;%d;%d", &type, &value, &value2);

	if (ret == 3) {
		if (!Key_GetCatcher()) {
			Sys_QueEvent(0, (sysEventType_t)type, value, value2, 0, 0);
		}
	} else {
		Com_Printf("WARNING AI_AgentMessage: ignoring malformed message: %s\n", msg);
	}
}

void AI_RecvPacket( const netadr_t *from, const byte *data, int dataLen )
{
	char event[64];

	// if (!NET_CompareAdr(from, &ais.udpAgentAdr)) {
	if (memcmp(from, &ais.udpAgentAdr, sizeof(netadr_t))) {
		Com_Printf("UDP AI Agent connected from %s\n", NET_AdrToString(from));
		ais.udpAgentAdr = *from;
	}

	if (dataLen + 1 > (int)sizeof(event)) {
		Com_Printf("WARNING AI_RecvPacket: ignoring overside packet from AI Agent\n");
		return;
	}

	memcpy(event, data, dataLen);
	event[dataLen] = '\0';

	AI_AgentMessage(event);
}

qboolean AI_AcceptConnection( const netadr_t *from )
{
	Com_Printf("TCP AI Agent connected from %s\n", NET_AdrToString(from));
	AI_InitEventStream(&ais.evStream, ais.evBuf, sizeof(ais.evBuf), AI_AgentMessage);
	return qtrue;
}

void AI_RecvStreamData( const byte *data, int dataSize ) {
	if (AI_IsEventStreamOpen(&ais.evStream)) {
		AI_EventStreamWrite(&ais.evStream, (const char *)data, dataSize);
	}
}
