/* Copyright (C) 1994-2005 Revolution Software Ltd
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

#include "common/stdafx.h"
#include "sword2/sword2.h"
#include "sword2/defs.h"
#include "sword2/interpreter.h"
#include "sword2/logic.h"
#include "sword2/memory.h"
#include "sword2/resman.h"
#include "sword2/driver/d_draw.h"

namespace Sword2 {

int32 Logic::fnAddMenuObject(int32 *params) {
	// params:	0 pointer to a MenuObject structure to copy down

	_vm->addMenuObject((MenuObject *) _vm->_memory->decodePtr(params[0]));
	return IR_CONT;
}

int32 Logic::fnRefreshInventory(int32 *params) {
	// called from 'menu_look_or_combine' script in 'menu_master' object
	// to update the menu to display a combined object while George runs
	// voice-over. Note that 'object_held' must be set to the graphic of
	// the combined object

	// params:	none

	// can reset this now
	_scriptVars[COMBINE_BASE] = 0;

	// so that the icon in 'object_held' is coloured while the rest are
	// grey
	_vm->_examiningMenuIcon = true;
	_vm->buildMenu();
	_vm->_examiningMenuIcon = false;

	return IR_CONT;
}

void Sword2Engine::addMenuObject(MenuObject *obj) {
	assert(_totalTemp < TOTAL_engine_pockets);
	memcpy(&_tempList[_totalTemp], obj, sizeof(MenuObject));
	_totalTemp++;
}

/**
 * Create and start the inventory (bottom) menu
 */

void Sword2Engine::buildMenu(void) {
	uint32 i, j;

	// Clear the temporary inventory list, since we are going to build a
	// new one from scratch.

	for (i = 0; i < TOTAL_engine_pockets; i++)
		_tempList[i].icon_resource = 0;

	_totalTemp = 0;

	// Run the 'build_menu' script in the 'menu_master' object. This will
	// register all carried menu objects.

	uint32 null_pc = 0;
	char *menuScript = (char *) _resman->openResource(MENU_MASTER_OBJECT);
	_logic->runScript(menuScript, menuScript, &null_pc);
	_resman->closeResource(MENU_MASTER_OBJECT);

	// Create a new master list based on the old master inventory list and
	// the new temporary inventory list. The purpose of all this is, as
	// far as I can tell, that the new list is ordered in the same way as
	// the old list, with new objects added to the end of it.

	// Compare new with old. Anything in master thats not in new gets
	// removed from master - if found in new too, remove from temp

	for (i = 0; i < _totalMasters; i++) {
		bool found_in_temp = false;

		for (j = 0; j < TOTAL_engine_pockets; j++) {
			if (_masterMenuList[i].icon_resource == _tempList[j].icon_resource) {
				// We alread know about this object, so kill it
				// in the temporary list.
				_tempList[j].icon_resource = 0;
				found_in_temp = true;
				break;
			}
		}

		if (!found_in_temp) {
			// The object is in the master list, but not in the
			// temporary list. The player must have lost the object
			// since the last time we checked, so kill it in the
			// master list.
			_masterMenuList[i].icon_resource = 0;
		}
	}

	// Eliminate blank entries from the master list.

	_totalMasters = 0;

	for (i = 0; i < TOTAL_engine_pockets; i++) {
		if (_masterMenuList[i].icon_resource) {
			if (i != _totalMasters) {
				memcpy(&_masterMenuList[_totalMasters], &_masterMenuList[i], sizeof(MenuObject));
				_masterMenuList[i].icon_resource = 0;
			}
			_totalMasters++;
		}
	}

	// Add the new objects - i.e. the ones still in the temporary list but
	// not yet in the master list - to the end of the master.

	for (i = 0; i < TOTAL_engine_pockets; i++) {
		if (_tempList[i].icon_resource) {
			memcpy(&_masterMenuList[_totalMasters++], &_tempList[i], sizeof(MenuObject));
		}
	}

	// Initialise the menu from the master list.

	for (i = 0; i < 15; i++) {
		uint32 res = _masterMenuList[i].icon_resource;
		byte *icon = NULL;

		if (res) {
			bool icon_coloured;

			if (_examiningMenuIcon) {
				// When examining an object, that object is
				// coloured. The rest are greyed out.
				icon_coloured = (res == Logic::_scriptVars[OBJECT_HELD]);
			} else if (Logic::_scriptVars[COMBINE_BASE]) {
				// When combining two menu object (i.e. using
				// one on another), both are coloured. The rest
				// are greyed out.
				icon_coloured = (res == Logic::_scriptVars[OBJECT_HELD] || res == Logic::_scriptVars[COMBINE_BASE]);
			} else {
				// If an object is selected but we are not yet
				// doing anything with it, the selected object
				// is greyed out. The rest are coloured.
				icon_coloured = (res != Logic::_scriptVars[OBJECT_HELD]);
			}

			icon = _resman->openResource(res) + sizeof(StandardHeader);

			// The coloured icon is stored directly after the
			// greyed out one.

			if (icon_coloured)
				icon += (RDMENU_ICONWIDE * RDMENU_ICONDEEP);
		}

		_graphics->setMenuIcon(RDMENU_BOTTOM, i, icon);

		if (res)
			_resman->closeResource(res);
	}

	_graphics->showMenu(RDMENU_BOTTOM);
}

/**
 * Build a fresh system (top) menu.
 */

void Sword2Engine::buildSystemMenu(void) {
	uint32 icon_list[5] = {
		OPTIONS_ICON,
		QUIT_ICON,
		SAVE_ICON,
		RESTORE_ICON,
		RESTART_ICON
	};

	// Build them all high in full colour - when one is clicked on all the
	// rest will grey out.

	for (int i = 0; i < ARRAYSIZE(icon_list); i++) {
		byte *icon = _resman->openResource(icon_list[i]) + sizeof(StandardHeader);
		
		// The only case when an icon is grayed is when the player
		// is dead. Then SAVE is not available.

		if (!Logic::_scriptVars[DEAD] || icon_list[i] != SAVE_ICON)
			icon += (RDMENU_ICONWIDE * RDMENU_ICONDEEP);

		_graphics->setMenuIcon(RDMENU_TOP, i, icon);
		_resman->closeResource(icon_list[i]);
	}

	_graphics->showMenu(RDMENU_TOP);
}

} // End of namespace Sword2
