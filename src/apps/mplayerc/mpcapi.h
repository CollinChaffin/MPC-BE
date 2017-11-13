/*
 * (C) 2006-2017 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


 /*
 This file defines commands used for "MPC" API. To send commands
 to MPC-BE and receive playback notifications, first launch MPC-BE with the /slave command line
 argument followed by a HWnd handle used to receive notification:

 ..\bin\mpc-be /slave 125421

 After startup, MPC-BE sends a WM_COPYDATA message to host with COPYDATASTRUCT struct filled with:
 - dwData :  CMD_CONNECT
 - lpData :  Unicode string containing MPC-BE's main window handle

 To control MPC-BE, send WM_COPYDATA messages to Hwnd provided on connection. All messages should be
 formatted as null-terminated Unicode strings. For commands or notifications with multiple parameters,
 values are separated by |.
 If a string contains a |, it will be escaped with a \ so a \| is not a separator.

 Ex: When a file is opened, MPC-BE sends to host the "now playing" notification:
 - dwData :  CMD_NOWPLAYING
 - lpData :  title|author|description|filename|duration

 Ex: When a DVD is playing, use CMD_GETNOWPLAYING to get:
 - dwData :  CMD_NOWPLAYING
 - lpData :  dvddomain|titlenumber|numberofchapters|currentchapter|titleduration
 dvddomains: DVD - Stopped, DVD - FirstPlay, DVD - RootMenu, DVD - TitleMenu, DVD - Title
 */

#pragma once

enum MPC_LOADSTATE {
	MLS_CLOSED,
	MLS_LOADING,
	MLS_LOADED,
	MLS_CLOSING
};

enum MPC_PLAYSTATE {
	PS_PLAY   = 0,
	PS_PAUSE  = 1,
	PS_STOP   = 2,
	PS_UNUSED = 3
};

struct MPC_OSDDATA {
	int nMsgPos;       // screen position constant (see OSD_MESSAGEPOS constants)
	int nDurationMS;   // duration in milliseconds
	WCHAR strMsg[128]; // message to display thought OSD
};

//// MPC_OSDDATA.nMsgPos constants (for host side programming):
//typedef enum
//{
//    OSD_NOMESSAGE,
//    OSD_TOPLEFT,
//    OSD_TOPRIGHT,
//} OSD_MESSAGEPOS;

enum MPCAPI_COMMAND : unsigned int {
	// ==== Commands from MPC to host

	// Send after connection
	// Parameter 1 : MPC window handle (command should be send to this HWnd)
	CMD_CONNECT             = 0x50000000,

	// Send when opening or closing file
	// Parameter 1 : current state (see MPC_LOADSTATE enum)
	CMD_STATE               = 0x50000001,

	// Send when playing, pausing or closing file
	// Parameter 1 : current play mode (see MPC_PLAYSTATE enum)
	CMD_PLAYMODE            = 0x50000002,

	// Send after opening a new file
	// Parameter 1 : title
	// Parameter 2 : author
	// Parameter 3 : description
	// Parameter 4 : complete filename (path included)
	// Parameter 5 : duration in seconds
	CMD_NOWPLAYING          = 0x50000003,

	// List of subtitle tracks
	// Parameter 1 : Subtitle track name 0
	// Parameter 2 : Subtitle track name 1
	// ...
	// Parameter n : Active subtitle track, -1 if subtitles disabled
	//
	// if no subtitle track present, returns -1
	// if no file loaded, returns -2
	CMD_LISTSUBTITLETRACKS  = 0x50000004,

	// List of audio tracks
	// Parameter 1 : Audio track name 0
	// Parameter 2 : Audio track name 1
	// ...
	// Parameter n : Active audio track
	//
	// if no audio track present, returns -1
	// if no file loaded, returns -2
	CMD_LISTAUDIOTRACKS     = 0x50000005,

	// List of files in the playlist
	// Parameter 1 : file path 0
	// Parameter 2 : file path 1
	// ...
	// Parameter n : active file, -1 if no active file
	CMD_PLAYLIST            = 0x50000006,

	// Send current playback position in responce
	// of CMD_GETCURRENTPOSITION.
	// Parameter 1 : current position in seconds
	CMD_CURRENTPOSITION     = 0x50000007,

	// Send the current playback position after a jump.
	// (Automatically sent after a seek event).
	// Parameter 1 : new playback position (in seconds).
	CMD_NOTIFYSEEK          = 0x50000008,

	// Notify the end of current playback
	// (Automatically sent).
	// Parameter 1 : none.
	CMD_NOTIFYENDOFSTREAM   = 0x50000009,

	// Send version str
	// Parameter 1: MPC-BE's version
	CMD_VERSION             = 0x5000000A,

	// Send information about MPC-BE closing
	CMD_DISCONNECT          = 0x5000000B,

	// ==== Commands from host to MPC

	// Open new file
	// Parameter 1 : file path
	CMD_OPENFILE            = 0xA0000000,

	// Stop playback, but keep file / playlist
	CMD_STOP                = 0xA0000001,

	// Stop playback and close file / playlist
	CMD_CLOSEFILE           = 0xA0000002,

	// Pause or restart playback
	CMD_PLAYPAUSE           = 0xA0000003,

	// Unpause playback
	CMD_PLAY                = 0xA0000004,

	// Pause playback
	CMD_PAUSE               = 0xA0000005,

	// Add a new file to playlist (did not start playing)
	// Parameter 1 : file path
	CMD_ADDTOPLAYLIST       = 0xA0001000,

	// Remove all files from playlist
	CMD_CLEARPLAYLIST       = 0xA0001001,

	// Start playing playlist
	CMD_STARTPLAYLIST       = 0xA0001002,

	CMD_REMOVEFROMPLAYLIST  = 0xA0001003,   // TODO

	// Cue current file to specific position
	// Parameter 1 : new position in seconds
	CMD_SETPOSITION         = 0xA0002000,

	// Set the audio delay
	// Parameter 1 : new audio delay in ms
	CMD_SETAUDIODELAY       = 0xA0002001,

	// Set the subtitle delay
	// Parameter 1 : new subtitle delay in ms
	CMD_SETSUBTITLEDELAY    = 0xA0002002,

	// Set the active file in the playlist
	// Parameter 1 : index of the active file, -1 for no file selected
	// DOESN'T WORK
	CMD_SETINDEXPLAYLIST    = 0xA0002003,

	// Set the audio track
	// Parameter 1 : index of the audio track
	CMD_SETAUDIOTRACK       = 0xA0002004,

	// Set the subtitle track
	// Parameter 1 : index of the subtitle track, -1 for disabling subtitles
	CMD_SETSUBTITLETRACK    = 0xA0002005,

	// Ask for a list of the subtitles tracks of the file
	// return a CMD_LISTSUBTITLETRACKS
	CMD_GETSUBTITLETRACKS   = 0xA0003000,

	// Ask for a list of the audio tracks of the file
	// return a CMD_LISTAUDIOTRACKS
	CMD_GETAUDIOTRACKS      = 0xA0003001,

	// Ask for the properties of the current loaded file
	// return a CMD_NOWPLAYING
	CMD_GETNOWPLAYING       = 0xA0003002,

	// Ask for the current playlist
	// return a CMD_PLAYLIST
	CMD_GETPLAYLIST         = 0xA0003003,

	// Ask for the current playback position,
	// see CMD_CURRENTPOSITION.
	// Parameter 1 : current position in seconds
	CMD_GETCURRENTPOSITION  = 0xA0003004,

	// Jump forward/backward of N seconds,
	// Parameter 1 : seconds (negative values for backward)
	CMD_JUMPOFNSECONDS      = 0xA0003005,

	// Ask slave for version
	CMD_GETVERSION          = 0xA0003006,

	// Toggle FullScreen
	CMD_TOGGLEFULLSCREEN    = 0xA0004000,

	// Jump forward(medium)
	CMD_JUMPFORWARDMED      = 0xA0004001,

	// Jump backward(medium)
	CMD_JUMPBACKWARDMED     = 0xA0004002,

	// Increase Volume
	CMD_INCREASEVOLUME      = 0xA0004003,

	// Decrease volume
	CMD_DECREASEVOLUME      = 0xA0004004,

	// Shader toggle
	CMD_SHADER_TOGGLE       = 0xA0004005,

	// Close App
	CMD_CLOSEAPP            = 0xA0004006,

	// Set playing rate
	CMD_SETSPEED            = 0xA0004008,

	// show host defined OSD message string
	CMD_OSDSHOWMESSAGE      = 0xA0005000,
};
