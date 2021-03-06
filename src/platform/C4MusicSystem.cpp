/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* Handles Music.ocg and randomly plays songs */

#include <C4Include.h>
#include <C4MusicSystem.h>

#include <C4Window.h>
#include <C4MusicFile.h>
#include <C4Application.h>
#include <C4Random.h>
#include <C4Log.h>
#include <C4Game.h>
#include <C4GraphicsSystem.h>

C4MusicSystem::C4MusicSystem():
		Songs(NULL),
		SongCount(0),
		PlayMusicFile(NULL),
		Volume(100),
		FadeMusicFile(NULL)
#if AUDIO_TK == AUDIO_TK_OPENAL
		, alcDevice(NULL), alcContext(NULL)
#endif
{
}

C4MusicSystem::~C4MusicSystem()
{
	Clear();
}

#if AUDIO_TK == AUDIO_TK_OPENAL
void C4MusicSystem::SelectContext()
{
	alcMakeContextCurrent(alcContext);
}
#endif

bool C4MusicSystem::InitializeMOD()
{
#if AUDIO_TK == AUDIO_TK_FMOD
#ifdef _WIN32
	// Debug code
	switch (Config.Sound.FMMode)
	{
	case 0:
		FSOUND_SetOutput(FSOUND_OUTPUT_WINMM);
		break;
	case 1:
		FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
#ifdef USE_WIN32_WINDOWS
		FSOUND_SetHWND(Application.pWindow->hWindow);
#endif
		break;
	case 2:
		FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
		FSOUND_SetDriver(0);
		break;
	}
	FSOUND_SetMixer(FSOUND_MIXER_QUALITY_AUTODETECT);
#endif
	if (FSOUND_GetVersion() < FMOD_VERSION)
	{
		LogF("FMod: You are using the wrong DLL version!  You should be using %.02f", FMOD_VERSION);
		return false;
	}
	if (!FSOUND_Init(44100, 32, 0))
	{
		LogF("FMod: %s", FMOD_ErrorString(FSOUND_GetError()));
		return false;
	}
	// ok
	MODInitialized = true;
	return true;
#elif AUDIO_TK == AUDIO_TK_SDL_MIXER
	SDL_version compile_version;
	const SDL_version * link_version;
	MIX_VERSION(&compile_version);
	link_version=Mix_Linked_Version();
	LogF("SDL_mixer runtime version is %d.%d.%d (compiled with %d.%d.%d)",
	     link_version->major, link_version->minor, link_version->patch,
	     compile_version.major, compile_version.minor, compile_version.patch);
	if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_Init(SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE))
	{
		LogF("SDL: %s", SDL_GetError());
		return false;
	}
	//frequency, format, stereo, chunksize
	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024))
	{
		LogF("SDL_mixer: %s", SDL_GetError());
		return false;
	}
	MODInitialized = true;
	return true;
#elif AUDIO_TK == AUDIO_TK_OPENAL
	alcDevice = alcOpenDevice(NULL);
	if (!alcDevice)
	{
		LogF("Sound system: OpenAL create context error");
		return false;
	}
	alcContext = alcCreateContext(alcDevice, NULL);
	if (!alcContext)
	{
		LogF("Sound system: OpenAL create context error");
		return false;
	}
#ifndef __APPLE__
	if (!alutInitWithoutContext(NULL, NULL))
	{
		LogF("Sound system: ALUT init error");
		return false;
	}
#endif
	MODInitialized = true;
	return true;
#endif
	return false;
}

void C4MusicSystem::DeinitializeMOD()
{
#if AUDIO_TK == AUDIO_TK_FMOD
	FSOUND_StopSound(FSOUND_ALL); /* to prevent some hangs in FMOD */
#ifdef DEBUG
	Sleep(0);
#endif
	FSOUND_Close();
#elif AUDIO_TK == AUDIO_TK_SDL_MIXER
	Mix_CloseAudio();
	SDL_Quit();
#elif AUDIO_TK == AUDIO_TK_OPENAL
#ifndef __APPLE__
	alutExit();
#endif
	alcDestroyContext(alcContext);
	alcCloseDevice(alcDevice);
	alcContext = NULL;
	alcDevice = NULL;
#endif
	MODInitialized = false;
}

bool C4MusicSystem::Init(const char * PlayList)
{
	// init mod
	if (!MODInitialized && !InitializeMOD()) return false;

	// Might be reinitialisation
	ClearSongs();
	// Global music file
	LoadDir(Config.AtSystemDataPath(C4CFN_Music));
	// User music file
	LoadDir(Config.AtUserDataPath(C4CFN_Music));
	// read MoreMusic.txt
	LoadMoreMusic();
	// set play list
	if (PlayList) SetPlayList(PlayList); else SetPlayList(0);
	// set initial volume
	SetVolume(Config.Sound.MusicVolume);

	// ok
	return true;
}

bool C4MusicSystem::InitForScenario(C4Group & hGroup)
{
	// check if the scenario contains music
	bool fLocalMusic = false;
	StdStrBuf MusicDir;
	if (GrpContainsMusic(hGroup))
	{
		// clear global songs
		ClearSongs();
		fLocalMusic = true;
		// add songs
		MusicDir.Take(Game.ScenarioFile.GetFullName());
		LoadDir(MusicDir.getData());
		// log
		LogF(LoadResStr("IDS_PRC_LOCALMUSIC"), MusicDir.getData());
	}
	// check for music folders in group set
	C4Group *pMusicFolder = NULL;
	while ((pMusicFolder = Game.GroupSet.FindGroup(C4GSCnt_Music, pMusicFolder)))
	{
		if (!fLocalMusic)
		{
			// clear global songs
			ClearSongs();
			fLocalMusic = true;
		}
		// add songs
		MusicDir.Take(pMusicFolder->GetFullName());
		MusicDir.AppendChar(DirectorySeparator);
		MusicDir.Append(C4CFN_Music);
		LoadDir(MusicDir.getData());
		// log
		LogF(LoadResStr("IDS_PRC_LOCALMUSIC"), MusicDir.getData());
	}
	// no music?
	if (!SongCount) return false;
	// set play list
	SetPlayList(0);
	// ok
	return true;
}

void C4MusicSystem::Load(const char *szFile)
{
	// safety
	if (!szFile || !*szFile) return;
	C4MusicFile *NewSong=NULL;
#if AUDIO_TK == AUDIO_TK_OPENAL
	// openal: Only ogg supported
	const char *szExt = GetExtension(szFile);
	if (SEqualNoCase(szExt, "ogg")) NewSong = new C4MusicFileOgg;
#elif AUDIO_TK == AUDIO_TK_FMOD
	const char *szExt = GetExtension(szFile);
	// get type
	switch (GetMusicFileTypeByExtension(GetExtension(szFile)))
	{
	case MUSICTYPE_MOD:
		if (MODInitialized) NewSong = new C4MusicFileMOD;
		break;
	case MUSICTYPE_MP3:
		if (MODInitialized) NewSong = new C4MusicFileMP3;
		break;
	case MUSICTYPE_OGG:
		if (MODInitialized) NewSong = new C4MusicFileOgg;
		break;

	case MUSICTYPE_MID:
		if (MODInitialized)
			NewSong = new C4MusicFileMID;
		break;
	default: return; // safety
	}
#elif AUDIO_TK == AUDIO_TK_SDL_MIXER
	if (GetMusicFileTypeByExtension(GetExtension(szFile)) == MUSICTYPE_UNKNOWN) return;
	NewSong = new C4MusicFileSDL;
#endif
	// unrecognized type/mod not initialized?
	if (!NewSong) return;
	// init music file
	NewSong->Init(szFile);
	// add song to list (push back)
	C4MusicFile *pCurr = Songs;
	while (pCurr && pCurr->pNext) pCurr = pCurr->pNext;
	if (pCurr) pCurr->pNext = NewSong; else Songs = NewSong;
	NewSong->pNext = NULL;
	// count songs
	SongCount++;
}

void C4MusicSystem::LoadDir(const char *szPath)
{
	char Path[_MAX_FNAME + 1], File[_MAX_FNAME + 1];
	C4Group *pDirGroup = NULL;
	// split path
	SCopy(szPath, Path, _MAX_FNAME);
	char *pFileName = GetFilename(Path);
	SCopy(pFileName, File);
	*(pFileName - 1) = 0;
	// no file name?
	if (!File[0])
		// -> add the whole directory
		SCopy("*", File);
	// no wildcard match?
	else if (!SSearch(File, "*?"))
	{
		// then it's either a file or a directory - do the test with C4Group
		pDirGroup = new C4Group();
		if (!pDirGroup->Open(szPath))
		{
			// so it must be a file
			if (!pDirGroup->Open(Path))
			{
				// -> file/dir doesn't exist
				LogF("Music File not found: %s", szPath);
				delete pDirGroup;
				return;
			}
			// mother group is open... proceed with normal handling
		}
		else
		{
			// ok, set wildcard (load the whole directory)
			SCopy(szPath, Path);
			SCopy("*", File);
		}
	}
	// open directory group, if not already done so
	if (!pDirGroup)
	{
		pDirGroup = new C4Group();
		if (!pDirGroup->Open(Path))
		{
			LogF("Music File not found: %s", szPath);
			delete pDirGroup;
			return;
		}
	}
	// search file(s)
	char szFile[_MAX_FNAME + 1];
	pDirGroup->ResetSearch();
	while (pDirGroup->FindNextEntry(File, szFile))
	{
		char strFullPath[_MAX_FNAME + 1];
		sprintf(strFullPath, "%s%c%s", Path, DirectorySeparator, szFile);
		Load(strFullPath);
	}
	// free it
	delete pDirGroup;
}

void C4MusicSystem::LoadMoreMusic()
{
	StdStrBuf MoreMusicFile;
	// load MoreMusic.txt
	if (!MoreMusicFile.LoadFromFile(Config.AtUserDataPath(C4CFN_MoreMusic))) return;
	// read contents
	char *pPos = MoreMusicFile.getMData();
	while (pPos && *pPos)
	{
		// get line
		char szLine[1024 + 1];
		SCopyUntil(pPos, szLine, '\n', 1024);
		pPos = strchr(pPos, '\n'); if (pPos) pPos++;
		// remove leading whitespace
		char *pLine = szLine;
		while (*pLine == ' ' || *pLine == '\t' || *pLine == '\r') pLine++;
		// and whitespace at end
		char *p = pLine + strlen(pLine) - 1;
		while (*p == ' ' || *p == '\t' || *p == '\r') { *p = 0; --p; }
		// comment?
		if (*pLine == '#')
		{
			// might be a "directive"
			if (SEqual(pLine, "#clear"))
				ClearSongs();
			continue;
		}
		// try to load file(s)
		LoadDir(pLine);
	}
}

void C4MusicSystem::ClearSongs()
{
	Stop();
	while (Songs)
	{
		C4MusicFile *pFile = Songs;
		Songs = pFile->pNext;
		delete pFile;
	}
	SongCount = 0;
}

void C4MusicSystem::Clear()
{
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	// Stop a fadeout
	Mix_HaltMusic();
#endif
	ClearSongs();
	if (MODInitialized) { DeinitializeMOD(); }
}

void C4MusicSystem::Execute()
{
	// Execute music fading
	if (FadeMusicFile)
	{
		C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
		// Fading done?
		if (tNow >= FadeTimeEnd)
		{
			FadeMusicFile->Stop();
			FadeMusicFile = NULL;
			if (PlayMusicFile) PlayMusicFile->SetVolume(Volume);
		}
		else
		{
			// Fade process
			int fade_volume = 1000 * (tNow - FadeTimeStart) / (FadeTimeEnd - FadeTimeStart);
			FadeMusicFile->SetVolume(Volume * (1000 - fade_volume) / 1000);
			if (PlayMusicFile) PlayMusicFile->SetVolume(Volume * fade_volume / 1000);
		}
	}
	// Ensure a piece is played
#if AUDIO_TK != AUDIO_TK_SDL_MIXER
	if (!::Game.iTick35 || !Game.IsRunning)
#endif
	{
		if (!PlayMusicFile)
			Play();
		else
			PlayMusicFile->CheckIfPlaying();
	}
}

bool C4MusicSystem::Play(const char *szSongname, bool fLoop, int fadetime_ms)
{
	if (Game.IsRunning ? !Config.Sound.RXMusic : !Config.Sound.FEMusic)
		return false;

	C4MusicFile* NewFile = NULL;

	// Specified song name
	if (szSongname && szSongname[0])
	{
		// Search in list
		for (NewFile=Songs; NewFile; NewFile = NewFile->pNext)
		{
			char songname[_MAX_FNAME+1];
			SCopy(szSongname, songname); DefaultExtension(songname, "mid");
			if (SEqual(GetFilename(NewFile->FileName), songname))
				break;
			SCopy(szSongname, songname); DefaultExtension(songname, "ogg");
			if (SEqual(GetFilename(NewFile->FileName), songname))
				break;
		}
	}

	// Random song
	else
	{
		// try to find random song
		for (int i = 0; i <= 1000; i++)
		{
			int nmb = SafeRandom(Max(ASongCount / 2 + ASongCount % 2, ASongCount - SCounter));
			int j;
			for (j = 0, NewFile = Songs; NewFile; NewFile = NewFile->pNext)
				if (!NewFile->NoPlay)
					if (NewFile->LastPlayed == -1 || NewFile->LastPlayed < SCounter - ASongCount / 2)
					{
						j++;
						if (j > nmb) break;
					}
			if (NewFile) break;
		}

	}

	// File found?
	if (!NewFile)
		return false;

	LogF(LoadResStr("IDS_PRC_PLAYMUSIC"), GetFilename(NewFile->FileName));

	// Stop/Fade out old music
	bool is_fading = fadetime_ms && NewFile != PlayMusicFile && PlayMusicFile;
	if (!is_fading)
	{
		Stop();
	}
	else
	{
		C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
		if (FadeMusicFile)
		{
			if (FadeMusicFile == NewFile && FadeMusicFile->IsLooping() == fLoop && tNow < FadeTimeEnd)
			{
				// Fading back to a song while it wasn't fully faded out yet. Just swap our pointers and fix timings for that.
				FadeMusicFile = PlayMusicFile;
				PlayMusicFile = NewFile;
				FadeTimeEnd = tNow + fadetime_ms * (tNow - FadeTimeStart) / (FadeTimeEnd - FadeTimeStart);
				FadeTimeStart = FadeTimeEnd - fadetime_ms;
				return true;
			}
			else
			{
				// Fading to a third song while the previous wasn't faded out yet
				// That's pretty chaotic anyway, so just cancel the last song
				// Also happens if fading should already be done, in which case it won't harm to stop now
				// (It would stop on next call to Execute() anyway)
				// Also happens when fading back to the same song but loop status changes, but that should be really uncommon.
				FadeMusicFile->Stop();
			}
			
		}
		FadeMusicFile = PlayMusicFile;
		PlayMusicFile = NULL;
		FadeTimeStart = tNow;
		FadeTimeEnd = FadeTimeStart + fadetime_ms;
	}

	// Play new song
	if (!NewFile->Play(fLoop)) return false;
	PlayMusicFile = NewFile;
	NewFile->LastPlayed = SCounter++;
	Loop = fLoop;

	// Set volume
	PlayMusicFile->SetVolume(Volume * !is_fading);

	return true;
}

void C4MusicSystem::NotifySuccess()
{
	// nothing played?
	if (!PlayMusicFile) return;
	// loop?
	if (Loop)
		if (PlayMusicFile->Play())
			return;
	// stop
	Stop();
}

void C4MusicSystem::FadeOut(int fadeout_ms)
{
	// Kill any previous fading music and schedule current piece to fade
	if (PlayMusicFile)
	{
		if (FadeMusicFile) FadeMusicFile->Stop();
		FadeMusicFile = PlayMusicFile;
		PlayMusicFile = NULL;
		FadeTimeStart = C4TimeMilliseconds::Now();
		FadeTimeEnd = FadeTimeStart + fadeout_ms;
	}
}

bool C4MusicSystem::Stop()
{
	if (PlayMusicFile)
	{
		PlayMusicFile->Stop();
		PlayMusicFile=NULL;
	}
	if (FadeMusicFile)
	{
		FadeMusicFile->Stop();
		FadeMusicFile = NULL;
	}
	return true;
}

int C4MusicSystem::SetVolume(int iLevel)
{
	if (iLevel > 100) iLevel = 100;
	if (iLevel < 0) iLevel = 0;
	// Save volume for next file
	Volume = iLevel;
	// Tell it to the act file
	if (PlayMusicFile)
		PlayMusicFile->SetVolume(iLevel);
	return iLevel;
}

MusicType GetMusicFileTypeByExtension(const char* ext)
{
	if (SEqualNoCase(ext, "mid"))
		return MUSICTYPE_MID;
#if AUDIO_TK == AUDIO_TK_FMOD || AUDIO_TK == AUDIO_TK_SDL_MIXER
	else if (SEqualNoCase(ext, "xm") || SEqualNoCase(ext, "it") || SEqualNoCase(ext, "s3m") || SEqualNoCase(ext, "mod"))
		return MUSICTYPE_MOD;
#ifdef USE_MP3
	else if (SEqualNoCase(ext, "mp3"))
		return MUSICTYPE_MP3;
#endif
#endif
	else if (SEqualNoCase(ext, "ogg"))
		return MUSICTYPE_OGG;
	return MUSICTYPE_UNKNOWN;
}

bool C4MusicSystem::GrpContainsMusic(C4Group &rGrp)
{
	// search for known file extensions
	return           rGrp.FindEntry("*.mid")
#ifdef USE_MP3
	                 || rGrp.FindEntry("*.mp3")
#endif
	                 || rGrp.FindEntry("*.xm")
	                 || rGrp.FindEntry("*.it")
	                 || rGrp.FindEntry("*.s3m")
	                 || rGrp.FindEntry("*.mod")
	                 || rGrp.FindEntry("*.ogg");
}

int C4MusicSystem::SetPlayList(const char *szPlayList, bool fForceSwitch, int fadetime_ms)
{
	// reset
	C4MusicFile *pFile;
	for (pFile = Songs; pFile; pFile = pFile->pNext)
	{
		pFile->NoPlay = true;
		pFile->LastPlayed = -1;
	}
	ASongCount = 0;
	SCounter = 0;
	if (szPlayList && *szPlayList)
	{
		// match
		char szFileName[_MAX_FNAME + 1];
		for (int cnt = 0; SGetModule(szPlayList, cnt, szFileName, _MAX_FNAME); cnt++)
			for (pFile = Songs; pFile; pFile = pFile->pNext) if (pFile->NoPlay)
				if (WildcardMatch(szFileName, GetFilename(pFile->FileName)) || pFile->HasCategory(szFileName))
				{
					ASongCount++;
					pFile->NoPlay = false;
				}
	}
	else
	{
		// default: all files except the ones beginning with an at ('@')
		// Ignore frontend and credits music
		for (pFile = Songs; pFile; pFile = pFile->pNext)
			if (*GetFilename(pFile->FileName) != '@' &&
			    !SEqual2(GetFilename(pFile->FileName), "Credits.") &&
			    !SEqual2(GetFilename(pFile->FileName), "Frontend."))
			{
				ASongCount++;
				pFile->NoPlay = false;
			}
	}
	// Force switch of music if currently playing piece is not in list?
	if (fForceSwitch && PlayMusicFile && PlayMusicFile->NoPlay)
	{
		Play(NULL, false, fadetime_ms);
	}
	return ASongCount;
}

bool C4MusicSystem::ToggleOnOff()
{
	// // command key for music toggle pressed
	// use different settings for game/menu (lobby also counts as "menu", so go by Game.IsRunning-flag rather than startup)
	if (Game.IsRunning)
	{
		// game music
		Config.Sound.RXMusic = !Config.Sound.RXMusic;
		if (!Config.Sound.RXMusic) Stop(); else Play();
		::GraphicsSystem.FlashMessageOnOff(LoadResStr("IDS_CTL_MUSIC"), !!Config.Sound.RXMusic);
	}
	else
	{
		// game menu
		Config.Sound.FEMusic = !Config.Sound.FEMusic;
		if (!Config.Sound.FEMusic) Stop(); else Play();
	}
	// key processed
	return true;
}
