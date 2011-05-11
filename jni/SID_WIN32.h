/*
 *  SID_WIN32.h - 6581 emulation, WIN32 specific stuff
 *
 *  Frodo (C) 1994-1997,2002-2005 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <dsound.h>

#include "VIC.h"
#include "main.h"

#define FRAG_FREQ SCREEN_FREQ				// one frag per frame

#define FRAGMENT_SIZE (SAMPLE_FREQ/FRAG_FREQ)		// samples, not bytes
#define FRAG_INTERVAL (1000/FRAG_FREQ)			// in milliseconds
#define BUFFER_FRAGS FRAG_FREQ				// frags the in buffer
#define BUFFER_SIZE  (2*FRAGMENT_SIZE*BUFFER_FRAGS)	// bytes, not samples
#define MAX_LEAD_AVG BUFFER_FRAGS			// lead average count

// This won't hurt DirectX 2 but it will help when using the DirectX 3 runtime.
#if !defined(DSBCAPS_GETCURRENTPOSITION2)
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000
#endif

class DigitalPlayer {

public:
	virtual ~DigitalPlayer() = 0;
	virtual BOOL Ready() = 0;
	virtual int GetCurrentPosition() = 0;
	virtual void Write(void *buffer, int position, int length) = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;
};

DigitalPlayer::~DigitalPlayer()
{
}

class DirectSound: public DigitalPlayer {

public:
	DirectSound();
	~DirectSound();
	BOOL Ready();
	int GetCurrentPosition();
	void Write(void *buffer, int position, int length);
	void Pause();
	void Resume();

private:
	BOOL ready;
	LPDIRECTSOUND pDS;
	LPDIRECTSOUNDBUFFER pPrimaryBuffer;
	LPDIRECTSOUNDBUFFER pSoundBuffer;
};

class WaveOut: public DigitalPlayer {

public:
	WaveOut();
	~WaveOut();
	BOOL Ready();
	int GetCurrentPosition();
	void Write(void *buffer, int position, int length);
	void Pause();
	void Resume();

private:
	void UnprepareHeader(int index);
	void UnprepareHeaders();

private:
	BOOL ready;
	HWAVEOUT hWaveOut;
	char wave_buffer[BUFFER_SIZE];
	WAVEHDR wave_header[SCREEN_FREQ];
	int last_unprepared;
};

void DigitalRenderer::init_sound()
{
	ready = FALSE;
	sound_buffer = new SWORD[2*FRAGMENT_SIZE];
	ThePlayer = 0;
	to_output = 0;
	divisor = 0;
	lead = new int[MAX_LEAD_AVG];

	StartPlayer();
}

DigitalRenderer::~DigitalRenderer()
{
	StopPlayer();

	delete[] sound_buffer;
	delete[] lead;
}

void DigitalRenderer::StartPlayer()
{
	direct_sound = ThePrefs.DirectSound;
	if (ThePrefs.DirectSound)
		ThePlayer = new DirectSound;
	else
		ThePlayer = new WaveOut;
	ready = ThePlayer->Ready();
	sb_pos = 0;
	memset(lead, 0, sizeof(lead));
	lead_pos = 0;
}

void DigitalRenderer::StopPlayer()
{
	delete ThePlayer;
	ready = FALSE;
}

void DigitalRenderer::EmulateLine()
{
	if (!ready)
		return;

	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;

#if 0
	// Now see how many samples have to be added for this line.
	// XXX: This is too much computation here, precompute it.
	divisor += SAMPLE_FREQ;
	while (divisor >= 0)
		divisor -= TOTAL_RASTERS*SCREEN_FREQ, to_output++;

	// Calculate the sound data only when we have enough to fill
	// the buffer entirely.
	if (to_output < FRAGMENT_SIZE)
		return;
	to_output -= FRAGMENT_SIZE;

	VBlank();
#endif
}

void DigitalRenderer::VBlank()
{
	if (!ready)
		return;

	// Delete and recreate the player if preferences have changed.
	if (direct_sound != ThePrefs.DirectSound) {
		StopPlayer();
		StartPlayer();
	}

	// Convert latency preferences from milliseconds to frags.
	int lead_smooth = ThePrefs.LatencyAvg/FRAG_INTERVAL;
	int lead_hiwater = ThePrefs.LatencyMax/FRAG_INTERVAL;
	int lead_lowater = ThePrefs.LatencyMin/FRAG_INTERVAL;

	// Compute the current lead in frags.
	int current_position = ThePlayer->GetCurrentPosition();
	if (current_position == -1)
		return;
	int lead_in_bytes = (sb_pos - current_position + BUFFER_SIZE) % BUFFER_SIZE;
	if (lead_in_bytes >= BUFFER_SIZE/2)
		lead_in_bytes -= BUFFER_SIZE;
	int lead_in_frags = lead_in_bytes / int(2*FRAGMENT_SIZE);
	lead[lead_pos++] = lead_in_frags;
	if (lead_pos == lead_smooth)
		lead_pos = 0;

	// Compute the average lead in frags.
	int avg_lead = 0;
	for (int i = 0; i < lead_smooth; i++)
		avg_lead += lead[i];
	avg_lead /= lead_smooth;
	//Debug("lead = %d, avg = %d\n", lead_in_frags, avg_lead);

	// If we're getting too far ahead of the audio skip a frag.
	if (avg_lead > lead_hiwater) {
		for (int i = 0; i < lead_smooth; i++)
			lead[i]--;
		Debug("Skipping a frag...\n");
		return;
	}

	// Calculate one frag.
	int nsamples = FRAGMENT_SIZE;
	calc_buffer(sound_buffer, 2*FRAGMENT_SIZE);

	// If we're getting too far behind the audio add an extra frag.
	if (avg_lead < lead_lowater) {
		for (int i = 0; i < lead_smooth; i++)
			lead[i]++;
		Debug("Adding an extra frag...\n");
		calc_buffer(sound_buffer + FRAGMENT_SIZE, 2*FRAGMENT_SIZE);
		nsamples += FRAGMENT_SIZE;
	}

	// Write the frags to the player and update out write position. 
	ThePlayer->Write(sound_buffer, sb_pos, 2*nsamples);
	sb_pos = (sb_pos + 2*nsamples) % BUFFER_SIZE;
}

void DigitalRenderer::Pause()
{
	if (!ready)
		return;
	ThePlayer->Pause();
}

void DigitalRenderer::Resume()
{
	if (!ready)
		return;
	ThePlayer->Resume();
}

// Direct sound implemenation.

DirectSound::DirectSound()
{
	ready = FALSE;
	pDS = NULL;
	pPrimaryBuffer = NULL;
	pSoundBuffer = NULL;

	HRESULT dsrval = DirectSoundCreate(NULL, &pDS, NULL);
	if (dsrval != DS_OK) {
		DebugResult("DirectSoundCreate failed", dsrval);
		return;
	}

	// Set cooperative level trying to get exclusive or normal mode.
	DWORD level = ThePrefs.ExclusiveSound ? DSSCL_EXCLUSIVE : DSSCL_NORMAL;
	dsrval = pDS->SetCooperativeLevel(hwnd, level);
	if (dsrval != DS_OK) {
		DebugResult("SetCooperativeLevel failed", dsrval);
		return;
	}

	WAVEFORMATEX wfx;
	memset(&wfx, 0, sizeof(wfx));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = wfx.nChannels*wfx.wBitsPerSample/8;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign*wfx.nSamplesPerSec;
	wfx.cbSize = 0;

	DSBUFFERDESC dsbd;
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = NULL;

	dsrval = pDS->CreateSoundBuffer(&dsbd, &pPrimaryBuffer, NULL);
	if (dsrval != DS_OK) {
		DebugResult("CreateSoundBuffer for primary failed", dsrval);
		return;
	}

	dsrval = pPrimaryBuffer->SetFormat(&wfx);
	if (dsrval != DS_OK) {
		DebugResult("SetFormat on primary failed", dsrval);
		//return;
	}

	dsrval = pPrimaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
	if (dsrval != DS_OK) {
		DebugResult("Play primary failed", dsrval);
		return;
	}

	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwBufferBytes = BUFFER_SIZE;
	dsbd.lpwfxFormat = &wfx;

	dsrval = pDS->CreateSoundBuffer(&dsbd, &pSoundBuffer, NULL);
	if (dsrval != DS_OK) {
		DebugResult("CreateSoundBuffer failed", dsrval);
		return;
	}

	ready = TRUE;
}

DirectSound::~DirectSound()
{
	if (pDS != NULL) {
		if (pSoundBuffer != NULL) {
			pSoundBuffer->Release();
			pSoundBuffer = NULL;
		}
		if (pPrimaryBuffer != NULL) {
			pPrimaryBuffer->Release();
			pPrimaryBuffer = NULL;
		}
		pDS->Release();
		pDS = NULL;
	}
}

BOOL DirectSound::Ready()
{
	return ready;
}

int DirectSound::GetCurrentPosition()
{
	DWORD dwPlayPos, dwWritePos;
	HRESULT dsrval = pSoundBuffer->GetCurrentPosition(&dwPlayPos, &dwWritePos);
	if (dsrval != DS_OK) {
		DebugResult("GetCurrentPostion failed", dsrval);
		return -1;
	}
	return dwWritePos;
}

void DirectSound::Write(void *buffer, int position, int length)
{
	// Lock sound buffer.
	LPVOID pMem1, pMem2;
	DWORD dwSize1, dwSize2;
	HRESULT dsrval = pSoundBuffer->Lock(position, length,
		&pMem1, &dwSize1, &pMem2, &dwSize2, 0);
	if (dsrval != DS_OK) {
		DebugResult("Sound Lock failed", dsrval);
		return;
	}

	// Copy the sample buffer into the sound buffer.
	BYTE *pSample = (BYTE *) buffer;
	memcpy(pMem1, pSample, dwSize1);
	if (dwSize2 != 0)
		memcpy(pMem2, pSample + dwSize1, dwSize2);

	// Unlock the sound buffer.
	dsrval = pSoundBuffer->Unlock(pMem1, dwSize1, pMem2, dwSize2);
	if (dsrval != DS_OK) {
		DebugResult("Unlock failed\n", dsrval);
		return;
	}

	// Play the sound buffer.
	dsrval = pSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
	if (dsrval != DS_OK) {
		DebugResult("Play failed", dsrval);
		return;
	}
}

void DirectSound::Pause()
{
	HRESULT dsrval = pSoundBuffer->Stop();
	if (dsrval != DS_OK)
		DebugResult("Stop failed", dsrval);
	dsrval = pPrimaryBuffer->Stop();
	if (dsrval != DS_OK)
		DebugResult("Stop primary failed", dsrval);
}

void DirectSound::Resume()
{
	HRESULT dsrval = pPrimaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
	if (dsrval != DS_OK)
		DebugResult("Play primary failed", dsrval);
	dsrval = pSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
	if (dsrval != DS_OK)
		DebugResult("Play failed", dsrval);
}

// Wave output implemenation.

WaveOut::WaveOut()
{
	ready = FALSE;

	WAVEFORMATEX wfx;
	memset(&wfx, 0, sizeof(wfx));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = wfx.nChannels*wfx.wBitsPerSample/8;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign*wfx.nSamplesPerSec;
	wfx.cbSize = 0;
	MMRESULT worval = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, 0);
	if (worval != MMSYSERR_NOERROR) {
		Debug("waveOutOpen returned %d\n", worval);
		return;
	}
	memset(wave_header, 0, sizeof(wave_header));

	last_unprepared = 0;

	ready = TRUE;
}

WaveOut::~WaveOut()
{
	waveOutReset(hWaveOut);
	UnprepareHeaders();
	waveOutClose(hWaveOut);
}

BOOL WaveOut::Ready()
{
	return ready;
}

int WaveOut::GetCurrentPosition()
{
	MMTIME mmtime;
	memset(&mmtime, 0, sizeof(mmtime));
	mmtime.wType = TIME_BYTES;
	MMRESULT worval = waveOutGetPosition(hWaveOut, &mmtime, sizeof(mmtime));
	if (worval != MMSYSERR_NOERROR) {
		Debug("waveOutGetPosition(%d) returned %d\n", worval);
		return -1;
	}
	int current_position = mmtime.u.cb % BUFFER_SIZE;
	return current_position;
}

void WaveOut::Write(void *buffer, int position, int length)
{
	// If we are called for a double length buffer split it in half.
	if (length == 4*FRAGMENT_SIZE) {
		int half = length/2;
		Write(buffer, position, half);
		Write(((char *) buffer) + half, position + half, half);
		return;
	}

	// Free up as many previous frags as possible.
	for (;;) {
		WAVEHDR &wh = wave_header[last_unprepared];
		if (!(wh.dwFlags & WHDR_DONE))
			break;
		UnprepareHeader(last_unprepared);
		last_unprepared++;
		if (last_unprepared == BUFFER_FRAGS)
			last_unprepared = 0;
	}

	// Make sure the current header isn't in use.
	int index = position/(2*FRAGMENT_SIZE);
	WAVEHDR &wh = wave_header[index];
	if (wh.dwFlags & WHDR_DONE)
		UnprepareHeader(index);
	if (wh.dwFlags != 0) {
		Debug("wave header %d is in use!\n", index);
		return;
	}
	
	// Prepare the header and write the sound data.
	wh.lpData = wave_buffer + position;
	wh.dwBufferLength = length;
	wh.dwFlags = 0;
	memcpy(wh.lpData, buffer, length);
	MMRESULT worval = waveOutPrepareHeader(hWaveOut, &wh, sizeof(wh));
	if (worval != MMSYSERR_NOERROR) {
		Debug("waveOutPrepareHeader(%d) returned %d\n", index, worval);
		return;
	}
	worval = waveOutWrite(hWaveOut, &wh, sizeof(wh));
	if (worval != MMSYSERR_NOERROR) {
		Debug("waveOutWrite(%d) returned %d\n", index, worval);
		return;
	}
}

void WaveOut::Pause()
{
	waveOutPause(hWaveOut);
}

void WaveOut::Resume()
{
	waveOutRestart(hWaveOut);
}

void WaveOut::UnprepareHeader(int index)
{
	WAVEHDR &wh = wave_header[index];
	MMRESULT worval = waveOutUnprepareHeader(hWaveOut, &wh, sizeof(wh));
	if (worval != MMSYSERR_NOERROR) {
		Debug("waveOutUnprepareHeader(%d) returned %d\n", index, worval);
		return;
	}
	memset(&wh, 0, sizeof(wh));
}

void WaveOut::UnprepareHeaders()
{
	for (int i = 0; i < BUFFER_FRAGS; i++) {
		WAVEHDR &wh = wave_header[i];
		if (wh.dwFlags & WHDR_DONE)
			UnprepareHeader(i);
	}
}

#if 0

// Log player implemenation.

void Log::Write()
{
	// Dump the sound output to the log for debugging.
	{
		int last = sound_buffer[0];
		int count = 1;
		for (int i = 1; i < nsamples; i++) {
			if (sound_buffer[i] == last) {
				count++;
				continue;
			}
			Debug("[%dx%d] ", count, last);
			count = 1;
			last = sound_buffer[i];
		}
		Debug("[%dx%d] ", count, last);
	}
}

#endif

#if 0

// File player implemenation.

void Log::Write()
{
	// Log the sound output to a file for debugging.
	{
		static FILE *ofp = NULL;
		if (ofp == NULL) {
			ofp = fopen("debug.sid", "wb");
			if (ofp == NULL)
				Debug("fopen failed\n");
		}
		if (ofp != NULL) {
			Debug("Write sound data to file\n");
			if (fwrite(sound_buffer, 1, 2*nsamples, ofp) != 2*nsamples)
				Debug("fwrite failed\n");
		}
	}
}

#endif
