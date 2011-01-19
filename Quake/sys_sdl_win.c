/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2005 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "quakedef.h"

#include <sys/types.h>
#include <errno.h>
#include <io.h>
#include <direct.h>

#include "SDL.h"


qboolean		isDedicated;

static HANDLE		hinput, houtput;

#define	MAX_HANDLES		32	/* johnfitz -- was 10 */
static FILE		*sys_handles[MAX_HANDLES];


static int findhandle (void)
{
	int i;

	for (i = 1; i < MAX_HANDLES; i++)
	{
		if (!sys_handles[i])
			return i;
	}
	Sys_Error ("out of handles");
	return -1;
}

int Sys_filelength (FILE *f)
{
	int pos;
	int end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (const char *path, int *hndl)
{
	FILE	*f;
	int	i, retval;

	i = findhandle ();
	f = fopen(path, "rb");

	if (!f)
	{
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*hndl = i;
		retval = Sys_filelength(f);
	}

	return retval;
}

int Sys_FileOpenWrite (const char *path)
{
	FILE	*f;
	int		i;

	i = findhandle ();
	f = fopen(path, "wb");

	if (!f)
		Sys_Error ("Error opening %s: %s", path, strerror(errno));

	sys_handles[i] = f;
	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

void Sys_FileSeekRelative (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_CUR);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, const void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int Sys_FileTime (const char *path)
{
	FILE	*f;

	f = fopen(path, "rb");

	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
}

int Sys_FileTell (int handle)
{
    return ftell(sys_handles[handle]);
}

void Sys_Init (void)
{
	if (isDedicated)
	{
		if (!AllocConsole ())
		{
			isDedicated = false;	/* so that we have a graphical error dialog */
			Sys_Error ("Couldn't create dedicated server console");
		}

		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	}
}

void Sys_mkdir (const char *path)
{
	int rc = _mkdir (path);
	if (rc != 0 && errno != EEXIST)
		Sys_Error("Unable to create directory %s", path);
}

static const char errortxt1[] = "\nERROR-OUT BEGIN\n\n";
static const char errortxt2[] = "\nQUAKE ERROR: ";

void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[1024];
	DWORD		dummy;

	va_start (argptr, error);
	q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	if (isDedicated)
		WriteFile (houtput, errortxt1, strlen(errortxt1), &dummy, NULL);
	/* SDL will put these into its own stderr log,
	   so print to stderr even in graphical mode. */
	fputs (errortxt1, stderr);
	Host_Shutdown ();
	fputs (errortxt2, stderr);
	fputs (text, stderr);
	fputs ("\n\n", stderr);
	if (!isDedicated)
		PL_ErrorDialog(text);
	else
	{
		WriteFile (houtput, errortxt2, strlen(errortxt2), &dummy, NULL);
		WriteFile (houtput, text,      strlen(text),      &dummy, NULL);
		WriteFile (houtput, "\r\n",    2,		  &dummy, NULL);
		SDL_Delay (3000);	/* show the console 3 more seconds */
	}

// shut down QHOST hooks if necessary
//	DeinitConProc ();

	exit (1);
}

void Sys_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	DWORD		dummy;

	va_start (argptr,fmt);
	q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	if (isDedicated)
	{
		WriteFile(houtput, text, strlen(text), &dummy, NULL);
	}
	else
	{
	/* SDL will put these into its own stdout log,
	   so print to stdout even in graphical mode. */
		fputs (text, stdout);
	}
}

void Sys_Quit (void)
{
	Host_Shutdown();

	if (isDedicated)
		FreeConsole ();

	exit (0);
}

double Sys_FloatTime (void)
{
	return SDL_GetTicks() / 1000.0;
}

char *Sys_ConsoleInput (void)
{
	static char	con_text[256];
	static int		textlen;
	INPUT_RECORD	recs[1024];
	int		ch;
	DWORD		dummy, numread, numevents;

	if (!isDedicated)
		return NULL;	// no stdin necessary in graphical mode

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
				case '\r':
					WriteFile(houtput, "\r\n", 2, &dummy, NULL);

					if (textlen)
					{
						con_text[textlen] = 0;
						textlen = 0;
						return con_text;
					}

					break;

				case '\b':
					WriteFile(houtput, "\b \b", 3, &dummy, NULL);
					if (textlen)
					{
						textlen--;
					}
					break;

				default:
					if (ch >= ' ')
					{
						WriteFile(houtput, &ch, 1, &dummy, NULL);
						con_text[textlen] = ch;
						textlen = (textlen + 1) & 0xff;
					}

					break;
				}
			}
		}
	}

	return NULL;
}

void Sys_Sleep (void)
{
}

void Sys_SendKeyEvents (void)
{
	SDL_Event event;

	SDL_PumpEvents();
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			Key_Event(Key_Map(&(event.key)), event.key.type == SDL_KEYDOWN);
			return;
		case SDL_QUIT:
			Sys_Quit();
			break;
		default:
			SDL_PumpEvents();
			break;
		}
	}
}

void Sys_LowFPPrecision (void)
{
}

void Sys_HighFPPrecision (void)
{
}

void Sys_SetFPCW (void)
{
}
