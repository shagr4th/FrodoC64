/**
 * @file sid_epoc32.i - 6581 emulation, EPOC specific stuff
 *
 * (c) 2001-2002 Alfred E. Heggestad
 */

#ifdef __ER6__
#include <e32hal.h>
#include "frodo_appview.h"
#else
#include "alaw.h"
#endif


// #define PCM_DUMP

/**
 * Buffer size: 2^9 == 512 bytes. Note that too large buffers will not work
 * very well: The speed of the C64 is slowed down to an average speed of 
 * 100% by the blocking write() call in EmulateLine(). If you use a buffer 
 * of, say 4096 bytes, that will happen only about every 4 frames, which
 * means that the emulation runs much faster in some frames, and much
 * slower in others.
 * On really fast machines, it might make sense to use an even smaller
 * buffer size.
 */

#ifdef __ER6__
/**
 * This should always be more than 160 so that there is never more than one
 * sample block per screen copied to audio stream.
 */
const TInt KDefaultBufSize = 256;
const TInt KMaxSoundBufSize = 2048;
#elif defined(__SERIES60__) || define (__UIQ__)
/**
 * This should always be more than 320 so that there is never more than one
 * sample block per screen copied to audio stream.
 */
const TInt KDefaultBufSize = 512;
const TInt KMaxSoundBufSize = 4096;
#else
const TInt KDefaultBufSize = 512;
#endif

void DigitalRenderer::init_sound()
/**
 *  Initialization
 */
	{
#ifdef __ER6__

	ELOG1(_L8("DigitalRenderer::init_sound()\n"));
	ready = false; // assume the worst
	divisor = to_output = buffer_pos = 0;
#ifdef FRODO_DEBUG_VIEW
	iLastVolumeSet = 0;
#endif
	the_c64->iSoundHaveBeenPaused = ETrue; // Causes reset_sync() to be called

	TTimeIntervalMicroSeconds32 period;
	UserHal::TickPeriod(period);
	iTickPeriod_ys = period.Int();
    iBlockDuration_ys = (1000000 * KDefaultBufSize) / SAMPLE_FREQ;
    iBlockDurationInTics = iBlockDuration_ys / iTickPeriod_ys;

	sound_buffer = new int16[KMaxSoundBufSize];
	sound_buffer_desc.Set((TUint8 *)sound_buffer, KDefaultBufSize*2);

	/*
	 * open audio stream
	 */
	iSettings.iCaps = 0;			
	iSettings.iMaxVolume = 0;		

	iStream = CMdaAudioOutputStream::NewL(*this);
	iStream->Open(&iSettings);

	the_c64->iIsAudioPending = ETrue;

#else // __ER6__

	divisor = to_output = buffer_pos = 0;
	ready = false; // asume the worst

	/*
	 * open sound device
	 */
	TInt ret = iDevSound.Open();
	if (ret != KErrNone)
		{
		ELOG2(_L8("AUDIO: could not open sound device [ret=%d]\n"), ret);
		return;
		}

	/*
	 * get sound device capabilites
	 */ 
	TSoundCaps sndCaps;
	iDevSound.Caps(sndCaps);
	ELOG1(_L8("AUDIO: RDevSound - caps:\n"));
	ELOG2(_L8("  iService      = 0x%08x\n"), sndCaps().iService);
	ELOG2(_L8("  iVolume       = 0x%08x\n"), sndCaps().iVolume);
	ELOG2(_L8("  iMaxVolume    = 0x%08x\n"), sndCaps().iMaxVolume);
	ELOG2(_L8("  iMaxFrequency = 0x%08x\n"), sndCaps().iMaxFrequency);

	/**
	 * configure sound device
	 */
	TSoundConfig sndConfig;
	iDevSound.Config(sndConfig); // read the default config

	sndConfig().iVolume = EVolumeByValue; // 4
//	sndConfig().iVolumeValue = sndCaps().iMaxVolume; @todo
	sndConfig().iVolumeValue = 1;
	sndConfig().iAlawBufferSize = KDefaultBufSize * 2; // double buffering
	
	ret = iDevSound.SetConfig(sndConfig);
	if(ret != KErrNone)
		{
		ELOG2(_L8("AUDIO: WARNING! Could not set config due to [%d]\n"), ret);
		iDevSound.Close();
		return;
		}

	ELOG2(_L8("AUDIO: iVolume         %d \n"), sndConfig().iVolume );
	ELOG2(_L8("       iAlawBufferSize %d \n"), sndConfig().iAlawBufferSize );

	/**
	 * prepare the buffer
	 */
	ret = iDevSound.PreparePlayAlawBuffer(); // This *must* be done before reading the Buffer size!!!
	if(ret != KErrNone)
		{
		ELOG2(_L8("AUDIO: WARNING! could not prepare alaw buffer due to [%d]\n"), ret);
		return;
		}

	sound_buffer = new int16[KDefaultBufSize];
	iAlawBuffer = HBufC8::New(KDefaultBufSize);
	if(!sound_buffer || !iAlawBuffer)
		{
		ELOG1(_L8("AUDIO: no memory left for audio buffers :(\n"));
		iDevSound.Close();
		return;
		}

	ELOG1(_L8("AUDIO: the SID chip is ready to Rock'n'Roll...\n"));
	ready = true;

#endif // __ER6__
	}


void DigitalRenderer::reset_sync()
/**
 * Reset audio synchronizing
 */
	{
	iCountSampleBlocksCopied = 0;
	iStartPlayTickCount = User::TickCount(); 
	iAudioStarvation = EFalse;
	iLddBufferFullCounter = 0;
	iIsLddBufferFull = EFalse;
	iMomentOfWriteInTics = 0;

#ifdef FRODO_DEBUG_VIEW
	// For debugging
	CE32FrodoAppView* appView = the_c64->iFrodoPtr->iDocPtr->iAppUiPtr->iAppView;
	appView->iSidDebugData = this;
	iTickPeriod_ys2 = iTickPeriod_ys;
	iLeadInBlocks = 0;
	iCountSamplesCopied = 0;
	iPrevCountSamplesCopied = 0;
	iCountDuplicatedBlocks = 0;
	iCountSkippedBlocks = 0;
	iCountNormalBlocks = 0;
	iCountExtraBlocks = 0;
	iPlayCompleteCounter = 0;
	iBufferLengthTooBigCounter = 0;
	iPrevTickCount = 0;
	iMaxCopyTime = 0;
	iMinCopyTime = 0;
#endif
	}


DigitalRenderer::~DigitalRenderer()
/**
 *  Destructor
 */
	{
	ELOG1(_L8("DigitalRenderer::~DigitalRenderer"));
#ifdef __ER6__

	if (iStream)
		{
		delete iStream;
		ELOG1(_L8("DigitalRenderer::~DigitalRenderer: iStream deleted"));
		}
	delete sound_buffer;

#ifdef FRODO_DEBUG_VIEW
	// For debugging
	CE32FrodoAppView* appView = the_c64->iFrodoPtr->iDocPtr->iAppUiPtr->iAppView;
	appView->iSidDebugData = NULL;
#endif

#else // !__ER6__

	delete iAlawBuffer;
	delete sound_buffer;
	iDevSound.Close();

#endif
	ELOG1(_L8("DigitalRenderer::~DigitalRenderer END"));
	}


void DigitalRenderer::Pause(void)
/**
 *  Pause sound output (do nothing)
 */
	{
	/*
	if (!ready)
		return;
	iStream->Stop();
	*/
	}


void DigitalRenderer::Resume(void)
/**
 * Resume sound output (do nothing)
 */
	{
	/*
	//reset_sync();
	the_c64->iIsAudioPending = EFalse;
	*/
	}


void DigitalRenderer::EmulateLine(void)
/**
 * Fill buffer, sample volume (for sampled voice)
 */
	{
	if (!ready)
		return;

	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;

	/*
	 * Now see how many samples have to be added for this line
	 */
	divisor += SAMPLE_FREQ;
	while (divisor >= 0)
		divisor -= TOTAL_RASTERS*SCREEN_FREQ, to_output++;

	/*
	 * Calculate the sound data only when we have enough to fill
	 * the buffer entirely.
	 */
	if ((buffer_pos + to_output) >= KDefaultBufSize)
		{
		int datalen = KDefaultBufSize - buffer_pos;
		to_output -= datalen;
		calc_buffer(sound_buffer + buffer_pos, datalen*2);

#ifdef PCM_DUMP

		_LIT(KPcmDumpFile, "c:\\e32frodo_pcm.dat");
		const TInt KFileOpenWritable = EFileWrite | EFileShareAny | EFileStream;
		
		RFs fs;
		TInt ret = fs.Connect();
		if( (ret == KErrNone) || (ret == KErrAlreadyExists) )
			{
			RFile file;
			ret = file.Open(fs, KPcmDumpFile, KFileOpenWritable);
			if(ret == KErrNotFound)
				ret = file.Replace(fs, KPcmDumpFile, KFileOpenWritable);
			if(ret == KErrNone)
				{
				TInt pos;
				ret = file.Seek(ESeekEnd, pos);
				if (ret==KErrNone)
					{
					TPtrC8 ptr((unsigned char*)sound_buffer, KDefaultBufSize*2); // 2x8 = 16
					file.Write(ptr);
					}
				else
					{
					ELOG2(_L8("PCMDUMP: could not seek due to %d\n"), ret);
					}
				
				file.Close();
				}
			else
				{
				ELOG2(_L8("PCMDUMP: could not open file due to %d\n"), ret);
				}
			
			fs.Close();
			}

#else // PCM_DUMP

#ifdef __ER6__

		/*
		 * Reset audio sync if there has been a pause
		 */
		if (the_c64->iSoundHaveBeenPaused)
			{
			the_c64->iSoundHaveBeenPaused = EFalse;
			reset_sync();
			}

		the_c64->debug_audio_ready_scanline_num = -1; //!!DEBUG: // Audio is beeing fed now

		/**
		 * Calc lead value in blocks. Target audio frequency is SAMPLE_FREQ Hz. 
		 */
		TUint elapsedTics = User::TickCount() - iStartPlayTickCount + 1; // Add one to get a fast start
		TUint elapsedBlocks_ys = elapsedTics * iTickPeriod_ys;

		// We could precalc some values to save cpu...
		// Add 0.5 (i.e. 500) for rounding. 
		TUint countElapsedBlocks  = (((elapsedBlocks_ys * (SAMPLE_FREQ/1000)) / KDefaultBufSize) + 500) / 1000;
		iLeadInBlocks = iCountSampleBlocksCopied - countElapsedBlocks;

		/*
		 * Synchronize audio: If we are not in sync, either duplicate or skip current sample block.
		 * If we are lagging too much (emu speed is < 50%), even duplicating every block is not enough 
		 * and there will be breaks in audio.
		 *
		 * @todo there might be some variable overflow problems if audio is running a long time without resync()...
		 */
		if ( (iLeadInBlocks < -ThePrefs.LatencyMax) || iAudioStarvation)
			{
			// Duplicate current block.
			Mem::Copy((TUint8 *)sound_buffer + KDefaultBufSize*2, sound_buffer, KDefaultBufSize*2);
			sound_buffer_desc.Set((TUint8 *)sound_buffer, 2 * KDefaultBufSize*2);
			
			if (iLeadInBlocks < -ThePrefs.LatencyMax)
				{
				iCountSampleBlocksCopied++;
				iCountSamplesCopied += KDefaultBufSize; // Only for debug !! TODO: Handle overflow!
				iCountDuplicatedBlocks++; // Only for debug
				}
			else
			
				{
				// Extra blocks are added because of starvation. Do not count these blocks in any other counters.
				iCountExtraBlocks++;
				iAudioStarvation = EFalse;
				}
			}
		else
			{
			sound_buffer_desc.Set((TUint8 *)sound_buffer, KDefaultBufSize*2);
			}
		if (!iIsLddBufferFull && (iLeadInBlocks <= ThePrefs.LatencyMax)) 
			{
			// No need to skip. Play current block.
			iStream->WriteL(sound_buffer_desc);
			iCountSampleBlocksCopied++;
			iCountNormalBlocks++;
			iCountSamplesCopied += KDefaultBufSize; // Only for debug !! TODO: Handle overflow!
			// Only one block at a time
			the_c64->iIsAudioPending = ETrue;
			iMomentOfWriteInTics = User::TickCount();
			}
		else
			{
			// Skipped
			if (iIsLddBufferFull)
				{
				iIsLddBufferFull = EFalse;
				//iCountSampleBlocksCopied += (-iLeadInBlocks + 50);
				}
			iCountSkippedBlocks++; // Only for debug
			}

#else // __ER6__

		//
		// convert from linear PCM to aLaw first
		//
		TPtr8 alawPtr = iAlawBuffer->Des();
		alawPtr.SetLength(KDefaultBufSize);
		Alaw::conv_s16bit_alaw((unsigned short*)sound_buffer, &alawPtr[0], KDefaultBufSize);
		
		//
		// and then play the Alaw buffer
		//
		TRequestStatus stat;
		iDevSound.PlayAlawData(stat, alawPtr);
		User::WaitForRequest(stat);
		TInt ret = stat.Int();
		if(ret != KErrNone)
			{
			ELOG2(_L8("AUDIO: Warning! could not play due to %d\n"), ret);
			}
#endif // __ER6__

#endif // PCM_DUMP

//
// this is how they do it on Linux:
//
//		write(devfd, sound_buffer, sndbufsize*2);

		buffer_pos = 0;
		}

	}

#ifdef __ER6__


void DigitalRenderer::MaoscOpenComplete(TInt aError)
/**
 * Initialize audio stream
 */
	{
	ELOG1(_L8("DigitalRenderer::MaoscOpenComplete()\n"));
#if defined(__SERIES60__) || defined (__UIQ__)
	iStream->SetAudioPropertiesL(TMdaAudioDataSettings::ESampleRate16000Hz, 
								 TMdaAudioDataSettings::EChannelsMono);
#else
	iStream->SetAudioPropertiesL(TMdaAudioDataSettings::ESampleRate8000Hz, 
								 TMdaAudioDataSettings::EChannelsMono);
#endif
	ready = true;
	the_c64->iIsAudioPending = EFalse;
	the_c64->iFrodoPtr->StartC64();

	SetVolumeL(ThePrefs.iVolume);
	iStream->SetPriority(EPriorityNormal, EMdaPriorityPreferenceNone);
	ELOG1(_L8("DigitalRenderer::MaoscOpenComplete() end\n"));
	}


void DigitalRenderer::MaoscBufferCopied(TInt aError, const TDesC8& aBuffer)
/**
 * Notify that the block has been copied from the client queue to audio server
 */
	{

	// Check if writing a sample has taken longer than assumed. It indicates
	// that LDD buffer is full and the server have to wait until there
	// is free space(?). In practice, normally (buffer is not full) copying takes 0 or 1 tics.
	// If buffer is full, there comes about 10-50 copies that are 0 or 1 ticks and then
	// one copy that takes about 50 ticks(!), then again 10-15 normal copies and one
	// big one, etc.
	//
	if (iMomentOfWriteInTics)
		{
		TUint copyTime = User::TickCount() - iMomentOfWriteInTics;
		iMaxCopyTime = Max((TInt)iMaxCopyTime, (TInt)copyTime);
		iMinCopyTime = Min((TInt)iMinCopyTime,(TInt)copyTime);
		TInt duration = Max(2, iBlockDurationInTics); // 0 or 1 tics are too small duration
		if (copyTime >= 3 /* duration*2 */)
			{
			iLddBufferFullCounter++;
			iIsLddBufferFull = ETrue;
			}
		}

	the_c64->iIsAudioPending = EFalse;
	the_c64->iFrodoPtr->StartC64();
	}


void DigitalRenderer::MaoscPlayComplete(TInt aError)
/**
 * Server has played all samples that it has.
 */
	{
	iPlayCompleteCounter++; // Only for debug
	iAudioStarvation = ETrue;
	}


void DigitalRenderer::SetVolumeL(TInt aVolume)
/**
 * Set audio volume
 */
	{
	ELOG1(_L8("DigitalRenderer::SetVolumeL"));
	if (!ready)
			return;
	TInt newVolume(aVolume);
	if(newVolume > 100)
		newVolume = iStream->MaxVolume();
	else if(newVolume < 0)
		newVolume = 0;
#ifdef FRODO_DEBUG_VIEW
	iLastVolumeSet = newVolume;
#endif
	newVolume = (newVolume*iStream->MaxVolume())/100;
	iStream->SetVolume(newVolume);
	ELOG1(_L8("DigitalRenderer::SetVolumeL END"));
	}

#endif


//
// this code is moved here from FixPoint.i to avoid global data
//

/// define as global variable (4x512=2048 bytes)
//FixPoint SinTable[(1<<ldSINTAB)];


#define FIXPOINT_SIN_COS_GENERIC \
  if (angle >= 3*(1<<ldSINTAB)) {return(-SinTable[(1<<(ldSINTAB+2)) - angle]);}\
  if (angle >= 2*(1<<ldSINTAB)) {return(-SinTable[angle - 2*(1<<ldSINTAB)]);}\
  if (angle >= (1<<ldSINTAB)) {return(SinTable[2*(1<<ldSINTAB) - angle]);}\
  return(SinTable[angle]);


// sin and cos: angle is fixpoint number 0 <= angle <= 2 (*PI)
inline FixPoint DigitalRenderer::fixsin(FixPoint x)
{
  int angle = x;

  angle = (angle >> (FIXPOINT_PREC - ldSINTAB - 1)) & ((1<<(ldSINTAB+2))-1);
  FIXPOINT_SIN_COS_GENERIC
}


inline FixPoint DigitalRenderer::fixcos(FixPoint x)
{
  int angle = x;

  // cos(x) = sin(x+PI/2)
  angle = (angle + (1<<(FIXPOINT_PREC-1)) >> (FIXPOINT_PREC - ldSINTAB - 1)) & ((1<<(ldSINTAB+2))-1);
  FIXPOINT_SIN_COS_GENERIC
}


inline void DigitalRenderer::InitFixSinTab(void)
{
 ELOG2(_L8("InitFixSinTab [SinTable=0x%08x] ..."), SinTable);
  int i;
  float step;

  for (i=0, step=0; i<(1<<ldSINTAB); i++, step+=0.5/(1<<ldSINTAB))
  {
    SinTable[i] = FixNo(sin(M_PI * step));
  }
  ELOG1(_L8("[  OK  ]\n"));
}
