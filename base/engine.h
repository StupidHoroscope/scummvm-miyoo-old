/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2003 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

#ifndef ENGINE_H
#define ENGINE_H

#include "common/scummsys.h"
#include "common/str.h"
#include "common/system.h"

extern const char *gScummVMVersion;		// e.g. "0.4.1"
extern const char *gScummVMBuildDate;	// e.g. "2003-06-24"
extern const char *gScummVMFullVersion;	// e.g. "ScummVM 0.4.1 (2003-06-24)"

class SoundMixer;
class GameDetector;
class Timer;
struct GameSettings;

class Engine {
public:
	OSystem *_system;
	SoundMixer *_mixer;
	Timer * _timer;

protected:
	const Common::String _gameDataPath;

public:
	Engine(GameDetector *detector, OSystem *syst);
	virtual ~Engine();

	// Invoke the main engine loop using this method
	virtual void go() = 0;

	// Get the save game dir path
	const char *getSavePath() const;

	// Specific for each engine preparare of erroe string
	virtual void errorString(const char *buf_input, char *buf_output) = 0;
};

extern Engine *g_engine;

#endif

