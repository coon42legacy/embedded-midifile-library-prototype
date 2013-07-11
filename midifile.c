/*
 * midiFile.c - A general purpose midi file handling library. This code
 *				can read and write MIDI files in formats 0 and 1.
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *          Copyright 1998-2010, Steven Goodwin
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef  __APPLE__
#include <malloc.h>
#endif
#include "midifile.h"

static FILE* g_file;


static void read_mem_from_pos(void* dst, DWORD pos, DWORD length)
{
	fseek(g_file, pos, SEEK_SET);
	fread(dst, 1, length, g_file);
}

static DWORD read_dword_value_from_pos(DWORD pos)
{
	DWORD ret = 0;
	read_mem_from_pos(&ret, pos, sizeof(DWORD));

	return ret;
}

static WORD read_word_value_from_pos(DWORD pos)
{
	WORD ret = 0;
	read_mem_from_pos(&ret, pos, sizeof(WORD));

	return ret;
}

static BYTE read_byte_value_from_pos(DWORD pos)
{
	BYTE ret = 0;
	read_mem_from_pos(&ret, pos, sizeof(BYTE));

	return ret;
}

static void read_string_from_pos_s(void* dst, DWORD pos, DWORD max_length)
{
	read_mem_from_pos(dst, pos, max_length - 2);
	((BYTE*)dst)[max_length - 1] = '\0'; // if the input sting is too long, just cut it.
}





/*
** Internal Functions
*/
#define DT_DEF				32			/* assume maximum delta-time + msg is no more than 32 bytes */
#define SWAP_WORD(w)		(WORD)(((w)>>8)|((w)<<8))
#define SWAP_DWORD(d)		(DWORD)((d)>>24)|(((d)>>8)&0xff00)|(((d)<<8)&0xff0000)|(((d)<<24))

#define _VAR_CAST				_MIDI_FILE *pMF = (_MIDI_FILE *)_pMF
#define IsFilePtrValid(pMF)		(pMF)
#define IsTrackValid(_x)		(_midiValidateTrack(pMF, _x))
#define IsChannelValid(_x)		((_x)>=1 && (_x)<=16)
#define IsNoteValid(_x)			((_x)>=0 && (_x)<128)
#define IsMessageValid(_x)		((_x)>=msgNoteOff && (_x)<=msgMetaEvent)


static BOOL _midiValidateTrack(const _MIDI_FILE *pMF, int iTrack)
{
	if (!IsFilePtrValid(pMF))	return FALSE;
	
	if (pMF->bOpenForWriting)
	{
		if (iTrack < 0 || iTrack >= MAX_MIDI_TRACKS)
			return FALSE;
	}
	else	/* open for reading */
	{
	/*
	if (!pMF->ptr)
		return FALSE;
	*/

	if (iTrack < 0 || iTrack>=pMF->Header.iNumTracks)
		return FALSE;
	}
	
	return TRUE;
}


static int _midiGetLength(int ppqn, int iNoteLen, BOOL bOverride)
{
int length = ppqn;
	
	if (bOverride)
		{
		length = iNoteLen;
		}
	else
		{
		switch(iNoteLen)
			{
			case	MIDI_NOTE_DOTTED_MINIM:
						length *= 3;
						break;

			case	MIDI_NOTE_DOTTED_CROCHET:
						length *= 3;
						length /= 2;
						break;

			case	MIDI_NOTE_DOTTED_QUAVER:
						length *= 3;
						length /= 4;
						break;

			case	MIDI_NOTE_DOTTED_SEMIQUAVER:
						length *= 3;
						length /= 8;
						break;

			case	MIDI_NOTE_DOTTED_SEMIDEMIQUAVER:
						length *= 3;
						length /= 16;
						break;

			case	MIDI_NOTE_BREVE:
						length *= 4;
						break;

			case	MIDI_NOTE_MINIM:
						length *= 2;
						break;

			case	MIDI_NOTE_QUAVER:
						length /= 2;
						break;

			case	MIDI_NOTE_SEMIQUAVER:
						length /= 4;
						break;

			case	MIDI_NOTE_SEMIDEMIQUAVER:
						length /= 8;
						break;
			
			case	MIDI_NOTE_TRIPLE_CROCHET:
						length *= 2;
						length /= 3;
						break;			
			}
		}
	
	return length;
}


static int		midiFileSetTracksDefaultChannel(_MIDI_FILE *_pMF, int iTrack, int iChannel)
{
int prev;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return 0;
	if (!IsTrackValid(iTrack))				return 0;
	if (!IsChannelValid(iChannel))			return 0;

	/* For programmer each, iChannel is between 1 & 16 - but MIDI uses
	** 0-15. Thus, the fudge factor of 1 :)
	*/
	prev = pMF->Track[iTrack].iDefaultChannel+1;
	pMF->Track[iTrack].iDefaultChannel = (BYTE)(iChannel-1);
	return prev;
}

static int		midiFileGetTracksDefaultChannel(const _MIDI_FILE *_pMF, int iTrack)
{
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return 0;
	if (!IsTrackValid(iTrack))				return 0;

	return pMF->Track[iTrack].iDefaultChannel+1;
}

static int		midiFileSetPPQN(_MIDI_FILE *_pMF, int PPQN)
{
int prev;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return MIDI_PPQN_DEFAULT;
	prev = pMF->Header.PPQN;
	pMF->Header.PPQN = (WORD)PPQN;
	return prev;
}

static int		midiFileGetPPQN(const _MIDI_FILE *_pMF)
{
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return MIDI_PPQN_DEFAULT;
	return (int)pMF->Header.PPQN;
}

static int		midiFileSetVersion(_MIDI_FILE *_pMF, int iVersion)
{
int prev;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return MIDI_VERSION_DEFAULT;
	if (iVersion<0 || iVersion>2)			return MIDI_VERSION_DEFAULT;
	prev = pMF->Header.iVersion;
	pMF->Header.iVersion = (WORD)iVersion;
	return prev;
}

static int			midiFileGetVersion(const _MIDI_FILE *_pMF)
{
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return MIDI_VERSION_DEFAULT;
	return pMF->Header.iVersion;
}



void midiFileOpen( _MIDI_FILE* pMF, const char *pFilename, BOOL* open_success )
{
	DWORD ptr2;
	BOOL bValidFile = FALSE;
	BYTE magic[4];

	g_file = fopen(pFilename, "rb");

	if(g_file)
	{
		pMF->ptr2 = 0;
		ptr2 = pMF->ptr2;
		read_mem_from_pos(magic, ptr2, 4); // read magic sequence

		// Is this a valid MIDI file ?
		if (magic[0] == 'M' && magic[1] == 'T' &&  magic[2] == 'h' && magic[3] == 'd')
		{
			DWORD dwData2;
			WORD wData2;
			int i;

			dwData2 = read_dword_value_from_pos(ptr2 + 4);
			pMF->Header.iHeaderSize = SWAP_DWORD(dwData2);

			wData2 = read_word_value_from_pos(ptr2 + 8);
			pMF->Header.iVersion = (WORD)SWAP_WORD(wData2);
					
			wData2 = read_word_value_from_pos(ptr2 + 10);
			pMF->Header.iNumTracks = (WORD)SWAP_WORD(wData2);
					
			wData2 = read_word_value_from_pos(ptr2 + 12);
			pMF->Header.PPQN = (WORD)SWAP_WORD(wData2);
					
			ptr2 += pMF->Header.iHeaderSize + 8;
			/*
			**	 Get all tracks
			*/
			for(i=0; i < MAX_MIDI_TRACKS; ++i)
			{
				pMF->Track[i].pos = 0;
				pMF->Track[i].last_status = 0;
			}
					
			for(i=0; i < (pMF->Header.iNumTracks < MAX_MIDI_TRACKS ? pMF->Header.iNumTracks : MAX_MIDI_TRACKS); ++i)
			{
				pMF->Track[i].pBase2 = ptr2;
				pMF->Track[i].ptr2 = ptr2 + 8;
				dwData2 = read_dword_value_from_pos(ptr2 + 4);

				pMF->Track[i].size = SWAP_DWORD(dwData2);
				pMF->Track[i].pEnd2 = ptr2 + pMF->Track[i].size + 8;
				ptr2 += pMF->Track[i].size + 8;
			}
						   
			pMF->bOpenForWriting = FALSE;
			pMF->pFile = NULL;
			bValidFile = TRUE;
		}
	}
	
	if (!bValidFile)
		*open_success = FALSE;
	else
		*open_success = TRUE;
}

typedef struct {
		int	iIdx;
		int	iEndPos;
		} MIDI_END_POINT;

static int qs_cmp_pEndPoints(const void *e1, const void *e2)
{
MIDI_END_POINT *p1 = (MIDI_END_POINT *)e1;
MIDI_END_POINT *p2 = (MIDI_END_POINT *)e2;

	return p1->iEndPos-p2->iEndPos;
}

/*
static BOOL	midiFileSyncTracks(_MIDI_FILE *_pMF, int iTrack1, int iTrack2)
{
int p1, p2;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))			return FALSE;
	if (!IsTrackValid(iTrack1))			return FALSE;
	if (!IsTrackValid(iTrack2))			return FALSE;

	p1 = pMF->Track[iTrack1].pos + pMF->Track[iTrack1].dt;
	p2 = pMF->Track[iTrack2].pos + pMF->Track[iTrack2].dt;
	
	if (p1 < p2)		midiTrackIncTime(pMF, iTrack1, p2-p1, TRUE);
	else if (p2 < p1)	midiTrackIncTime(pMF, iTrack2, p1-p2, TRUE);
	
	return TRUE;
}
*/

BOOL midiFileClose(_MIDI_FILE *_pMF)
{
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))			return FALSE;


	if (pMF->pFile)
		return fclose(pMF->pFile)?FALSE:TRUE;
	// free((void *)pMF); // this is not on heap anymore. it's now on the stack
	return TRUE;
}



static DWORD _midiReadVarLen2(DWORD ptr2, DWORD *num)
{
	register DWORD value = read_byte_value_from_pos(ptr2++);
	register BYTE c;

	
	if(value & 0x80)
	{
		value &= 0x7f;
		do
		{
			c = read_byte_value_from_pos(ptr2++);

			value = (value << 7) + (c & 0x7f);
		} 
		while (c & 0x80);
	}
	*num = value;

	return ptr2;
}

static BOOL _midiReadTrackCopyData2(MIDI_MSG *pMsg, DWORD ptr2, DWORD sz, BOOL bCopyPtrData)
{
	if (sz > pMsg->data_sz)
	{
		printf("sz was bigger: %d > %d\r\n", sz, pMsg->data_sz);

		pMsg->data = (BYTE *)realloc(pMsg->data, sz); // also acts as malloc. can be tolerated since it only allocs a few bytes
		pMsg->data_sz = sz;
	}

	if (!pMsg->data)
		return FALSE;

	if (bCopyPtrData && read_byte_value_from_pos(ptr2))
		read_mem_from_pos(pMsg->data, ptr2, sz);

	return TRUE;
}

int midiReadGetNumTracks(const _MIDI_FILE *_pMF)
{
	_VAR_CAST;
	return pMF->Header.iNumTracks;
}


BOOL midiReadGetNextMessage(const _MIDI_FILE *_pMF, int iTrack, MIDI_MSG *pMsg)
{
	MIDI_FILE_TRACK *pTrack;
	DWORD bptr2, pMsgDataPtr2;

	int sz;

	BYTE bTmp[16];

	_VAR_CAST;

	// just remove and assume the track is valid?
	if (!IsTrackValid(iTrack))
		return FALSE;
	
	pTrack = &pMF->Track[iTrack];

	/* FIXME: Check if there is data on this track first!!!	*/
	if (pTrack->ptr2 >= pTrack->pEnd2)
		return FALSE;
	
	pTrack->ptr2 = _midiReadVarLen2(pTrack->ptr2, &pMsg->dt);
	pTrack->pos += pMsg->dt;
	pMsg->dwAbsPos = pTrack->pos;

	bTmp[0] = read_byte_value_from_pos(pTrack->ptr2);
	if(bTmp[0] & 0x80) /* Is this is sys message */
	{
		pMsg->iType = (tMIDI_MSG)(bTmp[0] & 0xf0);
		pMsgDataPtr2 = pTrack->ptr2 + 1;

		/* SysEx & Meta events don't carry channel info, but something
		** important in their lower bits that we must keep */
		if (pMsg->iType == 0xf0)
		{
			pMsg->iType = (tMIDI_MSG)read_byte_value_from_pos(pTrack->ptr2);
		}
	}
	else						/* just data - so use the last msg type */
	{
		pMsg->iType = pMsg->iLastMsgType;
		pMsgDataPtr2 = pTrack->ptr2;
	}
	
	pMsg->iLastMsgType = (tMIDI_MSG)pMsg->iType;

	bTmp[0] = read_byte_value_from_pos(pTrack->ptr2);

	pMsg->iLastMsgChnl = (BYTE)((bTmp[0]     ) & 0x0f) + 1;

	bTmp[0] = read_byte_value_from_pos(pMsgDataPtr2 + 0);
	bTmp[1] = read_byte_value_from_pos(pMsgDataPtr2 + 1);
	bTmp[2] = read_byte_value_from_pos(pMsgDataPtr2 + 2);

	switch(pMsg->iType)
	{
	case	msgNoteOn:
		pMsg->MsgData.NoteOn.iChannel = pMsg->iLastMsgChnl;
		pMsg->MsgData.NoteOn.iNote = bTmp[0];
		pMsg->MsgData.NoteOn.iVolume = bTmp[1];

		pMsg->iMsgSize = 3;
		break;

	case	msgNoteOff:
		pMsg->MsgData.NoteOff.iChannel = pMsg->iLastMsgChnl;
		pMsg->MsgData.NoteOff.iNote = bTmp[0];

		pMsg->iMsgSize = 3;
		break;

	case	msgNoteKeyPressure:
		pMsg->MsgData.NoteKeyPressure.iChannel = pMsg->iLastMsgChnl;
		pMsg->MsgData.NoteKeyPressure.iNote = bTmp[0];
		pMsg->MsgData.NoteKeyPressure.iPressure = bTmp[1];

		pMsg->iMsgSize = 3;
		break;

	case	msgSetParameter:
		pMsg->MsgData.NoteParameter.iChannel = pMsg->iLastMsgChnl;
		pMsg->MsgData.NoteParameter.iControl = (tMIDI_CC)bTmp[0]; 
		pMsg->MsgData.NoteParameter.iParam = bTmp[1];
		pMsg->iMsgSize = 3;
		break;

	case	msgSetProgram:
		pMsg->MsgData.ChangeProgram.iChannel = pMsg->iLastMsgChnl;
		pMsg->MsgData.ChangeProgram.iProgram = bTmp[0];
		pMsg->iMsgSize = 2;
		break;

	case	msgChangePressure:
		pMsg->MsgData.ChangePressure.iChannel = pMsg->iLastMsgChnl;
		pMsg->MsgData.ChangePressure.iPressure = bTmp[0];
		pMsg->iMsgSize = 2;
		break;

	case	msgSetPitchWheel:
		pMsg->MsgData.PitchWheel.iChannel = pMsg->iLastMsgChnl;
		pMsg->MsgData.PitchWheel.iPitch = bTmp[0] | (bTmp[1] << 7);
		pMsg->MsgData.PitchWheel.iPitch -= MIDI_WHEEL_CENTRE;
		pMsg->iMsgSize = 3;
		break;

	case	msgMetaEvent:
		/* We can use 'pTrack->ptr' from now on, since meta events
		** always have bit 7 set */
		bptr2 = pTrack->ptr2;

		bTmp[1] = read_byte_value_from_pos(pTrack->ptr2 + 1);
		pMsg->MsgData.MetaEvent.iType = (tMIDI_META)bTmp[1];
		pTrack->ptr2 = _midiReadVarLen2(pTrack->ptr2 + 2, &pMsg->iMsgSize);

		bTmp[0] = read_byte_value_from_pos(pTrack->ptr2 + 0);
		bTmp[1] = read_byte_value_from_pos(pTrack->ptr2 + 1);
		bTmp[2] = read_byte_value_from_pos(pTrack->ptr2 + 2);
		bTmp[3] = read_byte_value_from_pos(pTrack->ptr2 + 3);
		bTmp[4] = read_byte_value_from_pos(pTrack->ptr2 + 4);

		sz = (pTrack->ptr2 - bptr2) + pMsg->iMsgSize;

		if (_midiReadTrackCopyData2(pMsg, pTrack->ptr2, sz, FALSE) == FALSE)
			return FALSE;

		/* Now copy the data...*/
		read_mem_from_pos(pMsg->data, bptr2, sz);

		/* Now place it in a neat structure */
		switch(pMsg->MsgData.MetaEvent.iType)
			{
			case	metaMIDIPort:
					pMsg->MsgData.MetaEvent.Data.iMIDIPort = bTmp[0];

					break;
			case	metaSequenceNumber:
					pMsg->MsgData.MetaEvent.Data.iSequenceNumber = bTmp[0];
					break;
			case	metaTextEvent:
			case	metaCopyright:
			case	metaTrackName:
			case	metaInstrument:
			case	metaLyric:
			case	metaMarker:
			case	metaCuePoint:
					/* TODO - Add NULL terminator ??? */
					//pMsg->MsgData.MetaEvent.Data.Text.pData = pTrack->ptr2;
					read_string_from_pos_s(pMsg->MsgData.MetaEvent.Data.Text.pData, pTrack->ptr2, sizeof(pMsg->MsgData.MetaEvent.Data.Text.pData));
					break;
			case	metaEndSequence:
					/* NO DATA */
					break;
			case	metaSetTempo:
					{
					
					DWORD us = bTmp[0] << 16 | (bTmp[1] << 8 ) | bTmp[2];
					pMsg->MsgData.MetaEvent.Data.Tempo.iBPM = 60000000L/us;
					}
					break;
			case	metaSMPTEOffset:
					pMsg->MsgData.MetaEvent.Data.SMPTE.iHours = bTmp[0];
					pMsg->MsgData.MetaEvent.Data.SMPTE.iMins= bTmp[1];
					pMsg->MsgData.MetaEvent.Data.SMPTE.iSecs = bTmp[2];
					pMsg->MsgData.MetaEvent.Data.SMPTE.iFrames = bTmp[3];
					pMsg->MsgData.MetaEvent.Data.SMPTE.iFF = bTmp[4];
					break;
			case	metaTimeSig:
					pMsg->MsgData.MetaEvent.Data.TimeSig.iNom = bTmp[0];
					pMsg->MsgData.MetaEvent.Data.TimeSig.iDenom = bTmp[1] * MIDI_NOTE_MINIM;
					/* TODO: Variations without 24 & 8 */
					break;
			case	metaKeySig:
					//if (*pTrack->ptr & 0x80)
					if (bTmp[0] & 0x80)
						{
						/* Do some trendy sign extending in reverse :) */
						pMsg->MsgData.MetaEvent.Data.KeySig.iKey = ((256 - bTmp[0]) & keyMaskKey); 
						pMsg->MsgData.MetaEvent.Data.KeySig.iKey |= keyMaskNeg;
						}
					else
						{
						pMsg->MsgData.MetaEvent.Data.KeySig.iKey = (tMIDI_KEYSIG)(bTmp[0] & keyMaskKey);
						}
					if (bTmp[1]) 
						pMsg->MsgData.MetaEvent.Data.KeySig.iKey |= keyMaskMin;
					break;
			case	metaSequencerSpecific:
					pMsg->MsgData.MetaEvent.Data.Sequencer.iSize = pMsg->iMsgSize;
					read_string_from_pos_s(pMsg->MsgData.MetaEvent.Data.Sequencer.pData, pTrack->ptr2, sizeof(pMsg->MsgData.MetaEvent.Data.Sequencer.pData));

					break;
			}

		pTrack->ptr2 += pMsg->iMsgSize;

		pMsg->iMsgSize = sz;
		break;

	case	msgSysEx1:
	case	msgSysEx2:
		bptr2 = pTrack->ptr2;
		pTrack->ptr2 = _midiReadVarLen2(pTrack->ptr2 + 1, &pMsg->iMsgSize);
		sz = (pTrack->ptr2 - bptr2) + pMsg->iMsgSize;
							
		if (_midiReadTrackCopyData2(pMsg, pTrack->ptr2, sz, FALSE) == FALSE)
			return FALSE;

		/* Now copy the data... */
		read_mem_from_pos(pMsg->data, bptr2, sz);
		pTrack->ptr2 += pMsg->iMsgSize;

		pMsg->iMsgSize = sz;
		pMsg->MsgData.SysEx.pData = pMsg->data; // ok!
		pMsg->MsgData.SysEx.iSize = sz;
		break;
	}
	/*
	** Standard MIDI messages use a common copy routine
	*/
	pMsg->bImpliedMsg = FALSE;
	if ((pMsg->iType & 0xf0) != 0xf0)
	{
		bTmp[0] = read_byte_value_from_pos(pTrack->ptr2);

		if (bTmp[0] & 0x80) 
		{
		}
		else 
		{
			pMsg->bImpliedMsg = TRUE;
			pMsg->iImpliedMsg = pMsg->iLastMsgType;
			pMsg->iMsgSize--;
		}

		_midiReadTrackCopyData2(pMsg, pTrack->ptr2, pMsg->iMsgSize, TRUE);
		pTrack->ptr2 += pMsg->iMsgSize;
	}

	return TRUE;
}


// ok
void midiReadInitMessage(MIDI_MSG *pMsg)
{
	pMsg->data = NULL;
	pMsg->data_sz = 0;
	pMsg->bImpliedMsg = FALSE;
}


// ok!
void midiReadFreeMessage(MIDI_MSG *pMsg)
{
	if (pMsg->data)
		free((void *)pMsg->data);
	pMsg->data = NULL;
}

