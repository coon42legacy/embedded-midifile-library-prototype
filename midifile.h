#ifndef _MIDIFILE_H
#define _MIDIFILE_H

#include "midiinfo.h"		/* enumerations and constants for GM */

/*
 * midiFile.c -  Header file for Steevs MIDI Library
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *			Copyright 1998-2010, Steven Goodwin.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License,or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* 
** All functions start with one of the following prefixes:
**		midiFile*		For non-GM features that relate to the file, and have
**						no use once the file has been created, i.e. CreateFile
**						or SetTrack (those data is embedded into the file, but
**						not explicitly stored)
**		midiSong*		For operations that work across the song, i.e. SetTempo
**		midiTrack*		For operations on a specific track, i.e. AddNoteOn
*/

/*
** Types because we're dealing with files, and need to be careful
*/
typedef	unsigned char		BYTE;
typedef	unsigned short		WORD;
typedef	unsigned long		DWORD;
typedef int					BOOL;
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif


/*
** MIDI Constants
*/
#define MIDI_PPQN_DEFAULT		384
#define MIDI_VERSION_DEFAULT	1

/*
** MIDI Limits
*/

// This settings are very important for using on an embedded system!
// When using default values, this structure will need a memory ammount of ~130KB!
// Since the STM32 board has only 8KB RAM, this values have been limited.
// All 256 possible midi channels aren't needed on floppy drives, so 16 should be enough.
// Polyphony is not possible at all on floppies so the value has been changed to 4 (for overlapping tones).
// This settings will only need 1KB instead of ~130KB
//
// - coon

#define MAX_MIDI_TRACKS			16 // default: 256 
#define MAX_TRACK_POLYPHONY		 4 // 64

/*
** MIDI structures, accessibly externably
*/
/*
** Internal Data Structures
*/
typedef struct 	{
	BYTE note, chn;
	BYTE valid, p2;
	DWORD end_pos;
} MIDI_LAST_NOTE;

typedef struct 	{
	BYTE *ptr;
	BYTE *pBase;
	BYTE *pEnd;

	DWORD pos;
	DWORD dt;
	/* For Reading MIDI Files */
	DWORD size;						/* size of whole iTrack */
	/* For Writing MIDI Files */
	DWORD iBlockSize;				/* max size of track */
	BYTE iDefaultChannel;			/* use for write only */
	BYTE last_status;				/* used for running status */

	MIDI_LAST_NOTE LastNote[MAX_TRACK_POLYPHONY];
} MIDI_FILE_TRACK;

typedef struct 	{
	DWORD	iHeaderSize;
	/**/
	WORD	iVersion;		/* 0, 1 or 2 */
	WORD	iNumTracks;		/* number of tracks... (will be 1 for MIDI type 0) */
	WORD	PPQN;			/* pulses per quarter note */
} MIDI_HEADER;

typedef struct {
	FILE				*pFile;
	BOOL				bOpenForWriting;

	MIDI_HEADER			Header;
	BYTE *ptr;			/* to whole data block */
	DWORD file_sz;

	MIDI_FILE_TRACK		Track[MAX_MIDI_TRACKS];
} _MIDI_FILE;


//typedef	void 	_MIDI_FILE;
typedef struct {
					tMIDI_MSG	iType;

					DWORD		dt;		/* delta time */
					DWORD		dwAbsPos;
					DWORD		iMsgSize;

					BOOL		bImpliedMsg;
					tMIDI_MSG	iImpliedMsg;

					/* Raw data chunk */
					BYTE *data;		/* dynamic data block */
					DWORD data_sz;
					
					union {
						struct {
								int			iNote;
								int			iChannel;
								int			iVolume;
								} NoteOn;
						struct {
								int			iNote;
								int			iChannel;
								} NoteOff;
						struct {
								int			iNote;
								int			iChannel;
								int			iPressure;
								} NoteKeyPressure;
						struct {
								int			iChannel;
								tMIDI_CC	iControl;
								int			iParam;
								} NoteParameter;
						struct {
								int			iChannel;
								int			iProgram;
								} ChangeProgram;
						struct {
								int			iChannel;
								int			iPressure;
								} ChangePressure;
						struct {
								int			iChannel;
								int			iPitch;
								} PitchWheel;
						struct {
								tMIDI_META	iType;
								union {
									int					iMIDIPort;
									int					iSequenceNumber;
									struct {
										BYTE			*pData;
										} Text;
									struct {
										int				iBPM;
										} Tempo;
									struct {
										int				iHours, iMins;
										int				iSecs, iFrames,iFF;
										} SMPTE;
									struct {
										tMIDI_KEYSIG	iKey;
										} KeySig;
									struct {
										int				iNom, iDenom;
										} TimeSig;
									struct {
										BYTE			*pData;
										int				iSize;
										} Sequencer;
									} Data;
								} MetaEvent;
						struct {
								BYTE		*pData;
								int			iSize;
								} SysEx;
						} MsgData;

				/* State information - Please treat these as private*/
				tMIDI_MSG	iLastMsgType;
				BYTE		iLastMsgChnl;
	
				} MIDI_MSG;

/*
** midiFile* Prototypes
*/
// _MIDI_FILE  *midiFileCreate(const char *pFilename, BOOL bOverwriteIfExists);
int			midiFileSetTracksDefaultChannel(_MIDI_FILE *pMF, int iTrack, int iChannel);
int			midiFileGetTracksDefaultChannel(const _MIDI_FILE *pMF, int iTrack);
BOOL		midiFileFlushTrack(_MIDI_FILE *pMF, int iTrack, BOOL bFlushToEnd, DWORD dwEndTimePos);
BOOL		midiFileSyncTracks(_MIDI_FILE *pMF, int iTrack1, int iTrack2);
int			midiFileSetPPQN(_MIDI_FILE *pMF, int PPQN);
int			midiFileGetPPQN(const _MIDI_FILE *pMF);
int			midiFileSetVersion(_MIDI_FILE *pMF, int iVersion);
int			midiFileGetVersion(const _MIDI_FILE *pMF);
void midiFileOpen(_MIDI_FILE* pMF, const char *pFilename, BOOL* open_success);
BOOL		midiFileClose(_MIDI_FILE *pMF);

/*
** midiSong* Prototypes
*/
BOOL		midiSongAddSMPTEOffset(_MIDI_FILE *pMF, int iTrack, int iHours, int iMins, int iSecs, int iFrames, int iFFrames);
BOOL		midiSongAddSimpleTimeSig(_MIDI_FILE *pMF, int iTrack, int iNom, int iDenom);
BOOL		midiSongAddTimeSig(_MIDI_FILE *pMF, int iTrack, int iNom, int iDenom, int iClockInMetroTick, int iNotated32nds);
BOOL		midiSongAddKeySig(_MIDI_FILE *pMF, int iTrack, tMIDI_KEYSIG iKey);
BOOL		midiSongAddTempo(_MIDI_FILE *pMF, int iTrack, int iTempo);
BOOL		midiSongAddMIDIPort(_MIDI_FILE *pMF, int iTrack, int iPort);
BOOL		midiSongAddEndSequence(_MIDI_FILE *pMF, int iTrack);

/*
** midiTrack* Prototypes
*/
BOOL		midiTrackAddRaw(_MIDI_FILE *pMF, int iTrack, int iDataSize, const BYTE *pData, BOOL bMovePtr, int iDeltaTime);
BOOL		midiTrackIncTime(_MIDI_FILE *pMF, int iTrack, int iDeltaTime, BOOL bOverridePPQN);
BOOL		midiTrackAddText(_MIDI_FILE *pMF, int iTrack, tMIDI_TEXT iType, const char *pTxt);
BOOL		midiTrackAddMsg(_MIDI_FILE *pMF, int iTrack, tMIDI_MSG iMsg, int iParam1, int iParam2);
BOOL		midiTrackSetKeyPressure(_MIDI_FILE *pMF, int iTrack, int iNote, int iAftertouch);
BOOL		midiTrackAddControlChange(_MIDI_FILE *pMF, int iTrack, tMIDI_CC iCCType, int iParam);
BOOL		midiTrackAddProgramChange(_MIDI_FILE *pMF, int iTrack, int iInstrPatch);
BOOL		midiTrackChangeKeyPressure(_MIDI_FILE *pMF, int iTrack, int iDeltaPressure);
BOOL		midiTrackSetPitchWheel(_MIDI_FILE *pMF, int iTrack, int iWheelPos);
BOOL		midiTrackAddNote(_MIDI_FILE *pMF, int iTrack, int iNote, int iLength, int iVol, BOOL bAutoInc, BOOL bOverrideLength);
BOOL		midiTrackAddRest(_MIDI_FILE *pMF, int iTrack, int iLength, BOOL bOverridePPQN);
BOOL		midiTrackGetEndPos(_MIDI_FILE *pMF, int iTrack);

/*
** midiRead* Prototypes
*/
int			midiReadGetNumTracks(const _MIDI_FILE *pMF);
BOOL		midiReadGetNextMessage(const _MIDI_FILE *pMF, int iTrack, MIDI_MSG *pMsg);
void		midiReadInitMessage(MIDI_MSG *pMsg);
void		midiReadFreeMessage(MIDI_MSG *pMsg);


#endif /* _MIDIFILE_H */

