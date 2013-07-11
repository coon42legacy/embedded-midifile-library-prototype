/*
 * mididump.c - A complete textual dump of a MIDI file.
 *				Requires Steevs MIDI Library & Utilities
 *				as it demonstrates the text name resolution code.
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *			Copyright 2010, Steven Goodwin.
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
#include <windows.h>
#include <mmsystem.h>
#include <time.h>
#include "midifile.h"
#include "midiutil.h"

#pragma comment (lib, "winmm.lib")

void HexList(BYTE *pData, int iNumBytes)
{
	int i;

	for(i=0;i<iNumBytes;i++)
		printf("%.2x ", pData[i]);
}

void playMidiFile(const char *pFilename)
{
	_MIDI_FILE pMF;
	BOOL open_success;
	char str[128];
	int ev;

	HMIDIOUT hMidiOut;
	int i = 0;
	clock_t t1, t2;

	unsigned int result = midiOutOpen(&hMidiOut, MIDI_MAPPER, 0, 0, 0);
	if(result != MMSYSERR_NOERROR)
	{
		printf("Midi device Geht nicht!");
	}

	midiFileOpen(&pMF, pFilename, &open_success);

	if (open_success)
	{
		static MIDI_MSG msg[MAX_MIDI_TRACKS];
		int i, iNum;
		unsigned int j;
		int any_track_had_data = 1;
		DWORD current_midi_tick = 0;
		DWORD bpm = 192;
		float ms_per_tick;
		DWORD ticks_to_wait = 0;

		iNum = midiReadGetNumTracks(&pMF);

		for(i=0; i< iNum; i++)
		{
			midiReadInitMessage(&msg[i]);
			midiReadGetNextMessage(&pMF, i, &msg[i]);
		}
		
		
		printf("start playing...\r\n");

		while(any_track_had_data)
		{
			any_track_had_data = 1;
			ticks_to_wait = -1;

			for(i=0; i < iNum; i++)
			{
				while(current_midi_tick == pMF.Track[i].pos && pMF.Track[i].ptr2 != pMF.Track[i].pEnd2)
				{
					//printf("[Track: %d]", i);

					if (msg[i].bImpliedMsg)
					{ ev = msg[i].iImpliedMsg; }
					else
					{ ev = msg[i].iType; }

					//printf(" %06d ", msg[i].dwAbsPos);


					if (muGetMIDIMsgName(str, ev))
						;//printf("%s  ", str);

					switch(ev)
					{
					case	msgNoteOff:
						muGetNameFromNote(str, msg[i].MsgData.NoteOff.iNote);
				//		printf("(%d) %s", msg[i].MsgData.NoteOff.iChannel, str);
						midiOutShortMsg(hMidiOut, (0 << 16) | (msg[i].MsgData.NoteOff.iNote << 8) | (0x80 + msg[i].MsgData.NoteOff.iChannel - 1) ); // note off
						break;
					case	msgNoteOn:
						muGetNameFromNote(str, msg[i].MsgData.NoteOn.iNote);
				//		printf("  (%d) %s %d", msg[i].MsgData.NoteOn.iChannel, str, msg[i].MsgData.NoteOn.iVolume);
						midiOutShortMsg(hMidiOut, (msg[i].MsgData.NoteOn.iVolume << 16) | (msg[i].MsgData.NoteOn.iNote << 8) | (0x90 + msg[i].MsgData.NoteOn.iChannel - 1) ); // note on	
						break;
					case	msgNoteKeyPressure:
						muGetNameFromNote(str, msg[i].MsgData.NoteKeyPressure.iNote);
						printf("(%d) %s %d", msg[i].MsgData.NoteKeyPressure.iChannel,
							str,
							msg[i].MsgData.NoteKeyPressure.iPressure);
						break;
					case	msgSetParameter:
						muGetControlName(str, msg[i].MsgData.NoteParameter.iControl);
						printf("(%d) %s -> %d", msg[i].MsgData.NoteParameter.iChannel,
							str, msg[i].MsgData.NoteParameter.iParam);
						break;
					case	msgSetProgram:
						midiOutShortMsg(hMidiOut, (0 << 16) | (msg[i].MsgData.ChangeProgram.iProgram << 8) | 0xC0 + msg[i].MsgData.ChangeProgram.iChannel); // set program

						muGetInstrumentName(str, msg[i].MsgData.ChangeProgram.iProgram);
						printf("(%d) %s", msg[i].MsgData.ChangeProgram.iChannel, str);
						break;
					case	msgChangePressure:
						muGetControlName(str, msg[i].MsgData.ChangePressure.iPressure);
						printf("(%d) %s", msg[i].MsgData.ChangePressure.iChannel, str);
						break;
					case	msgSetPitchWheel:
						//printf("(%d) %d", msg[i].MsgData.PitchWheel.iChannel,
							//msg[i].MsgData.PitchWheel.iPitch);
						break;

					case	msgMetaEvent:
						printf("---- ");
						switch(msg[i].MsgData.MetaEvent.iType)
						{
						case	metaMIDIPort:
							printf("MIDI Port = %d", msg[i].MsgData.MetaEvent.Data.iMIDIPort);
							break;

						case	metaSequenceNumber:
							printf("Sequence Number = %d",msg[i].MsgData.MetaEvent.Data.iSequenceNumber);
							break;

						case	metaTextEvent:
							printf("Text = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaCopyright:
							printf("Copyright = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaTrackName:
							printf("Track name = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaInstrument:
							printf("Instrument = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaLyric:
							printf("Lyric = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaMarker:
							printf("Marker = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaCuePoint:
							printf("Cue point = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaEndSequence:
							printf("End Sequence");
							break;
						case	metaSetTempo:
							bpm = msg[i].MsgData.MetaEvent.Data.Tempo.iBPM;
							ms_per_tick = 60000.0f / (bpm * pMF.Header.PPQN);
							printf("Tempo = %d", msg[i].MsgData.MetaEvent.Data.Tempo.iBPM);
							break;
						case	metaSMPTEOffset:
							printf("SMPTE offset = %d:%d:%d.%d %d",
								msg[i].MsgData.MetaEvent.Data.SMPTE.iHours,
								msg[i].MsgData.MetaEvent.Data.SMPTE.iMins,
								msg[i].MsgData.MetaEvent.Data.SMPTE.iSecs,
								msg[i].MsgData.MetaEvent.Data.SMPTE.iFrames,
								msg[i].MsgData.MetaEvent.Data.SMPTE.iFF
								);
							break;
						case	metaTimeSig:
							printf("Time sig = %d/%d",msg[i].MsgData.MetaEvent.Data.TimeSig.iNom,
								msg[i].MsgData.MetaEvent.Data.TimeSig.iDenom/MIDI_NOTE_CROCHET);
							break;
						case	metaKeySig:
							if (muGetKeySigName(str, msg[i].MsgData.MetaEvent.Data.KeySig.iKey))
								printf("Key sig = %s", str);
							break;

						case	metaSequencerSpecific:
							printf("Sequencer specific = ");
							HexList(msg[i].MsgData.MetaEvent.Data.Sequencer.pData, msg[i].MsgData.MetaEvent.Data.Sequencer.iSize); // ok
							printf("\r\n");
							break;
						}
						break;

					case	msgSysEx1:
					case	msgSysEx2:
						printf("Sysex = ");
						HexList(msg[i].MsgData.SysEx.pData, msg[i].MsgData.SysEx.iSize); // ok
						break;
					}

					if (ev == msgSysEx1 || ev == msgSysEx1 || (ev==msgMetaEvent && msg[i].MsgData.MetaEvent.iType==metaSequencerSpecific))
					{
						// Already done a hex dump
					}
					else
					{
						/*
						printf("  [");
						if (msg[i].bImpliedMsg) printf("%X!", msg[i].iImpliedMsg);
						for(j=0;j<msg[i].iMsgSize;j++)
							printf("%X ", msg[i].data[j]);
						printf("]\r\n");
						*/
					}

					if(midiReadGetNextMessage(&pMF, i, &msg[i]))
					{
						any_track_had_data = 1; // 0 ???
					}
				}

				ticks_to_wait = (pMF.Track[i].pos - current_midi_tick > 0 && ticks_to_wait > pMF.Track[i].pos - current_midi_tick) ? pMF.Track[i].pos - current_midi_tick : ticks_to_wait;
			}

			if(ticks_to_wait == -1)
				ticks_to_wait = 0;

			// wait microseconds per tick here
			t2 = clock() + ticks_to_wait * ms_per_tick;

			while( clock() < t2 )
			{
				// just wait here...
			}
			
			current_midi_tick += ticks_to_wait;
		}

		midiReadFreeMessage(&msg);
		midiFileClose(&pMF);


		printf("done.\r\n");
	}
	else
	{
		printf("Open Failed!\nInvalid MIDI-File Header!\n");

	}

	midiOutClose(hMidiOut);
}



int main(int argc, char* argv[])
{
	int i = 0;

	if (argc==1)
		printf("Usage: %s <filename>\n", argv[0]);
	else
	{
		for(i=1;i<argc;++i)
			playMidiFile(argv[i]);
	}

	return 0;
}
