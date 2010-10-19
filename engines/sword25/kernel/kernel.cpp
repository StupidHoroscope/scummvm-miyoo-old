/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

/*
 * This code is based on Broken Sword 2.5 engine
 *
 * Copyright (c) Malte Thiesen, Daniel Queteschiner and Michael Elsdoerfer
 *
 * Licensed under GNU GPL v2
 *
 */

#include "common/system.h"
#include "sword25/gfx/graphicengine.h"
#include "sword25/fmv/movieplayer.h"
#include "sword25/input/inputengine.h"
#include "sword25/kernel/kernel.h"
#include "sword25/kernel/persistenceservice.h"
#include "sword25/kernel/service_ids.h"
#include "sword25/package/packagemanager.h"
#include "sword25/script/script.h"
#include "sword25/sfx/soundengine.h"

namespace Sword25 {

#define BS_LOG_PREFIX "KERNEL"


Kernel *Kernel::_Instance = 0;

Kernel::Kernel() :
	_pWindow(NULL),
	_Running(false),
	_pResourceManager(NULL),
	_InitSuccess(false) {

	// Log that the kernel is beign created
	BS_LOGLN("created.");

	// Read the BS_SERVICE_TABLE and prepare kernel structures
	for (uint i = 0; i < ARRAYSIZE(BS_SERVICE_TABLE); i++) {
		// Is the superclass already registered?
		Superclass *pCurSuperclass = NULL;
		Common::Array<Superclass *>::iterator Iter;
		for (Iter = _superclasses.begin(); Iter != _superclasses.end(); ++Iter)
			if ((*Iter)->GetIdentifier() == BS_SERVICE_TABLE[i].superclassId) {
				pCurSuperclass = *Iter;
				break;
			}

		// If the superclass isn't already registered, then add it in
		if (!pCurSuperclass)
			_superclasses.push_back(new Superclass(this, BS_SERVICE_TABLE[i].superclassId));
	}

	// Create window object
	_pWindow = Window::CreateBSWindow(0, 0, 0, 0, false);
	if (!_pWindow) {
		BS_LOG_ERRORLN("Failed to create the window.");
	} else
		BS_LOGLN("Window created.");

	// Create the resource manager
	_pResourceManager = new ResourceManager(this);

	// Initialise the script engine
	ScriptEngine *pScript = static_cast<ScriptEngine *>(NewService("script", "lua"));
	if (!pScript || !pScript->init()) {
		_InitSuccess = false;
		return;
	}

	// Register kernel script bindings
	if (!_RegisterScriptBindings()) {
		BS_LOG_ERRORLN("Script bindings could not be registered.");
		_InitSuccess = false;
		return;
	}
	BS_LOGLN("Script bindings registered.");

	_InitSuccess = true;
}

Kernel::~Kernel() {
	// Services are de-registered in reverse order of creation
	while (!_ServiceCreationOrder.empty()) {
		Superclass *superclass = GetSuperclassByIdentifier(_ServiceCreationOrder.top());
		if (superclass) superclass->DisconnectService();
		_ServiceCreationOrder.pop();
	}

	// Empty the Superclass list
	while (_superclasses.size()) {
		delete _superclasses.back();
		_superclasses.pop_back();
	}

	// Release the window object
	delete _pWindow;
	BS_LOGLN("Window destroyed.");

	// Resource-Manager freigeben
	delete _pResourceManager;

	BS_LOGLN("destroyed.");
}

// Service Methoden
// ----------------

Kernel::Superclass::Superclass(Kernel *pKernel, const Common::String &Identifier) :
	_pKernel(pKernel),
	_Identifier(Identifier),
	_ServiceCount(0),
	_ActiveService(NULL) {
	for (uint i = 0; i < ARRAYSIZE(BS_SERVICE_TABLE); i++)
		if (BS_SERVICE_TABLE[i].superclassId == _Identifier)
			_ServiceCount++;
}

Kernel::Superclass::~Superclass() {
	DisconnectService();
}

/**
 * Gets the identifier of a service with a given superclass.
 * The number of services in a superclass can be learned with GetServiceCount().
 * @param superclassId      The name of the superclass
 *         e.g.: "sfx", "gfx", "package" ...
 * @param Number die Nummer des Services, dessen Bezeichner man erfahren will.<br>
 *         Hierbei ist zu beachten, dass der erste Service die Nummer 0 erh�lt. Number muss also eine Zahl zwischen
 *         0 und GetServiceCount() - 1 sein.
 */
Common::String Kernel::Superclass::GetServiceIdentifier(uint Number) {
	if (Number > _ServiceCount) return NULL;

	uint CurServiceOrd = 0;
	for (uint i = 0; i < ARRAYSIZE(BS_SERVICE_TABLE); i++) {
		if (BS_SERVICE_TABLE[i].superclassId == _Identifier) {
			if (Number == CurServiceOrd)
				return BS_SERVICE_TABLE[i].serviceId;
			else
				CurServiceOrd++;
		}
	}

	return Common::String();
}

/**
 * Creates a new service with the given identifier. Returns a pointer to the service, or null if the
 * service could not be created
 * Note: All services must be registered in service_ids.h, otherwise they cannot be created here
 * @param superclassId      The name of the superclass of the service
 *         e.g.: "sfx", "gfx", "package" ...
 * @param serviceId         The name of the service
 *         For the superclass "sfx" an example could be "Fmod" or "directsound"
 */
Service *Kernel::Superclass::NewService(const Common::String &serviceId) {
	for (uint i = 0; i < ARRAYSIZE(BS_SERVICE_TABLE); i++)
		if (BS_SERVICE_TABLE[i].superclassId == _Identifier &&
		        BS_SERVICE_TABLE[i].serviceId == serviceId) {
			Service *newService = BS_SERVICE_TABLE[i].create(_pKernel);

			if (newService) {
				DisconnectService();
				BS_LOGLN("Service '%s' created from superclass '%s'.", serviceId.c_str(), _Identifier.c_str());
				_ActiveService = newService;
				_ActiveServiceName = BS_SERVICE_TABLE[i].serviceId;
				return _ActiveService;
			} else {
				BS_LOG_ERRORLN("Failed to create service '%s' from superclass '%s'.", serviceId.c_str(), _Identifier.c_str());
				return NULL;
			}
		}

	BS_LOG_ERRORLN("Service '%s' is not avaliable from superclass '%s'.", serviceId.c_str(), _Identifier.c_str());
	return NULL;
}

/**
 * Ends the current service of a superclass. Returns true on success, and false if the superclass
 * does not exist or if not service was active
 * @param superclassId       The name of the superclass which is to be disconnected
 *         e.g.: "sfx", "gfx", "package" ...
 */
bool Kernel::Superclass::DisconnectService() {
	if (_ActiveService) {
		delete _ActiveService;
		_ActiveService = 0;
		BS_LOGLN("Active service '%s' disconnected from superclass '%s'.", _ActiveServiceName.c_str(), _Identifier.c_str());
		return true;
	}

	return false;
}

Kernel::Superclass *Kernel::GetSuperclassByIdentifier(const Common::String &Identifier) const {
	Common::Array<Superclass *>::const_iterator Iter;
	for (Iter = _superclasses.begin(); Iter != _superclasses.end(); ++Iter) {
		if ((*Iter)->GetIdentifier() == Identifier)
			return *Iter;
	}

	// BS_LOG_ERRORLN("Superclass '%s' does not exist.", Identifier.c_str());
	return NULL;
}

/**
 * Returns the number of register superclasses
 */
uint Kernel::GetSuperclassCount() const {
	return _superclasses.size();
}

/**
 * Returns the name of a superclass with the specified index.
 * Note: The number of superclasses can be retrieved using GetSuperclassCount
 * @param Number        The number of the superclass to return the identifier for.
 * It should be noted that the number should be between 0 und GetSuperclassCount() - 1.
 */
Common::String Kernel::GetSuperclassIdentifier(uint Number) const {
	if (Number > _superclasses.size())
		return NULL;

	uint CurSuperclassOrd = 0;
	Common::Array<Superclass *>::const_iterator Iter;
	for (Iter = _superclasses.begin(); Iter != _superclasses.end(); ++Iter) {
		if (CurSuperclassOrd == Number)
			return ((*Iter)->GetIdentifier());

		CurSuperclassOrd++;
	}

	return Common::String();
}

/**
 * Returns the number of services registered with a given superclass
 * @param superclassId      The name of the superclass
 *         e.g.: "sfx", "gfx", "package" ...
 */
uint Kernel::GetServiceCount(const Common::String &superclassId) const {
	Superclass *pSuperclass = GetSuperclassByIdentifier(superclassId);
	if (!pSuperclass)
		return 0;

	return pSuperclass->GetServiceCount();

}

/**
 * Gets the identifier of a service with a given superclass.
 * The number of services in a superclass can be learned with GetServiceCount().
 * @param superclassId      The name of the superclass
 *         e.g.: "sfx", "gfx", "package" ...
 * @param Number die Nummer des Services, dessen Bezeichner man erfahren will.<br>
 *         Hierbei ist zu beachten, dass der erste Service die Nummer 0 erh�lt. Number muss also eine Zahl zwischen
 *         0 und GetServiceCount() - 1 sein.
 */
Common::String Kernel::GetServiceIdentifier(const Common::String &superclassId, uint Number) const {
	Superclass *pSuperclass = GetSuperclassByIdentifier(superclassId);
	if (!pSuperclass)
		return NULL;

	return (pSuperclass->GetServiceIdentifier(Number));
}

/**
 * Creates a new service with the given identifier. Returns a pointer to the service, or null if the
 * service could not be created
 * Note: All services must be registered in service_ids.h, otherwise they cannot be created here
 * @param superclassId      The name of the superclass of the service
 *         e.g.: "sfx", "gfx", "package" ...
 * @param serviceId         The name of the service
 *         For the superclass "sfx" an example could be "Fmod" or "directsound"
 */
Service *Kernel::NewService(const Common::String &superclassId, const Common::String &serviceId) {
	Superclass *pSuperclass = GetSuperclassByIdentifier(superclassId);
	if (!pSuperclass)
		return NULL;

	// Die Reihenfolge merken, in der Services erstellt werden, damit sie sp�ter in umgekehrter Reihenfolge entladen werden k�nnen.
	_ServiceCreationOrder.push(superclassId);

	return pSuperclass->NewService(serviceId);
}

/**
 * Ends the current service of a superclass. 
 * @param superclassId       The name of the superclass which is to be disconnected
 *         e.g.: "sfx", "gfx", "package" ...
 * @return true on success, and false if the superclass does not exist or if not service was active.
 */
bool Kernel::DisconnectService(const Common::String &superclassId) {
	Superclass *pSuperclass = GetSuperclassByIdentifier(superclassId);
	if (!pSuperclass)
		return false;

	return pSuperclass->DisconnectService();
}

/**
 * Returns a pointer to the currently active service object of a superclass.
 * @param superclassId       The name of the superclass
 *         e.g.: "sfx", "gfx", "package" ...
 */
Service *Kernel::GetService(const Common::String &superclassId) {
	Superclass *pSuperclass = GetSuperclassByIdentifier(superclassId);
	if (!pSuperclass)
		return NULL;

	return (pSuperclass->GetActiveService());
}

/**
 * Returns the name of the currently active service object of a superclass.
 * If an error occurs, then an empty string is returned
 * @param superclassId       The name of the superclass
 *         e.g.: "sfx", "gfx", "package" ...
 */
Common::String Kernel::GetActiveServiceIdentifier(const Common::String &superclassId) {
	Superclass *pSuperclass = GetSuperclassByIdentifier(superclassId);
	if (!pSuperclass)
		return Common::String();

	return pSuperclass->GetActiveServiceName();
}

// -----------------------------------------------------------------------------

/**
 * Returns a random number
 * @param Min       The minimum allowed value
 * @param Max       The maximum allowed value
 */
int Kernel::GetRandomNumber(int Min, int Max) {
	BS_ASSERT(Min <= Max);

	return Min + _rnd.getRandomNumber(Max - Min + 1);
}

/**
 * Returns the elapsed time since startup in milliseconds
 */
uint Kernel::GetMilliTicks() {
	return g_system->getMillis();
}

// Other methods
// -----------------

/**
 * Returns how much memory is being used
 */
size_t Kernel::GetUsedMemory() {
	return 0;

#ifdef SCUMMVM_DISABLED_CODE
	PROCESS_MEMORY_COUNTERS pmc;
	pmc.cb = sizeof(pmc);
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		return pmc.WorkingSetSize;
	} else {
		BS_LOG_ERRORLN("Call to GetProcessMemoryInfo() failed. Error code: %d", GetLastError());
		return 0;
	}
#endif
}

// -----------------------------------------------------------------------------

/**
 * Returns a pointer to the active Gfx Service, or NULL if no Gfx service is active.
 */
GraphicEngine *Kernel::GetGfx() {
	return static_cast<GraphicEngine *>(GetService("gfx"));
}

// -----------------------------------------------------------------------------

/**
 * Returns a pointer to the active Sfx Service, or NULL if no Sfx service is active.
 */
SoundEngine *Kernel::GetSfx() {
	return static_cast<SoundEngine *>(GetService("sfx"));
}

// -----------------------------------------------------------------------------

/**
 * Returns a pointer to the active input service, or NULL if no input service is active.
 */
InputEngine *Kernel::GetInput() {
	return static_cast<InputEngine *>(GetService("input"));
}

// -----------------------------------------------------------------------------

/**
 * Returns a pointer to the active package manager, or NULL if no manager is active.
 */
PackageManager *Kernel::GetPackage() {
	return static_cast<PackageManager *>(GetService("package"));
}

// -----------------------------------------------------------------------------

/**
 * Returns a pointer to the script engine, or NULL if it is not active.
 */
ScriptEngine *Kernel::GetScript() {
	return static_cast<ScriptEngine *>(GetService("script"));
}

// -----------------------------------------------------------------------------

/**
 * Returns a pointer to the movie player, or NULL if it is not active.
 */
MoviePlayer *Kernel::GetFMV() {
	return static_cast<MoviePlayer *>(GetService("fmv"));
}

// -----------------------------------------------------------------------------

void Kernel::Sleep(uint Msecs) const {
	g_system->delayMillis(Msecs);
}

} // End of namespace Sword25
