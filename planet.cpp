
/*
 *
 * planet.cpp
 *
 * Created as planet.c on the 23rd of August 2005
 * Renamed planet.cpp on the 3rd of February 2009
 *
 * Part of the OpenJazz project
 *
 *
 * Copyright (c) 2005-2009 Alister Thomson
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/*
 * Deals with the loading, displaying and freeing of the planet landing
 * sequence.
 *
 */


#include "OpenJazz.h"


Planet::Planet (char * fn) {

	File *f;

	try {

		f = new File(fn, false);

	} catch (int e) {

		throw e;

	}

	// To do

	delete f;

	return;

}


Planet::~Planet () {

	// To do

	return;

}


int Planet::run () {

	while (true) {

		if (loop() == E_QUIT) return E_QUIT;

		if (controls[C_ESCAPE].state) {

			releaseControl(C_ESCAPE);

			return E_NONE;

		}

		// To do

	}

	return E_NONE;

}


