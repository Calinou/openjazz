
/*
 *
 * level.cpp
 *
 * Created as level.c on the 23rd of August 2005
 * Renamed level.cpp on the 3rd of February 2009
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
 * Deals with the running and freeing of ordinary levels.
 *
 */


#include "OpenJazz.h"
#include <string.h>


bool Level::checkMask (fixed x, fixed y) {

	// Anything off the edge of the map is solid
	if ((x < 0) || (y < 0) || (x > (LW * TW << 10)) || (y > (LH * TH << 10)))
		return true;

	// Event 122 is one-way
	if (grid[y >> 15][x >> 15].event == 122) return false;

	// Check the mask in the tile in question
	return mask[grid[y >> 15][x >> 15].tile][((y >> 9) & 56) + ((x >> 12) & 7)];

}


bool Level::checkMaskDown (fixed x, fixed y) {

	// Anything off the edge of the map is solid
	if ((x < 0) || (y < 0) || (x > (LW * TW << 10)) || (y > (LH * TH << 10)))
		return true;

	// Check the mask in the tile in question
	return mask[grid[y >> 15][x >> 15].tile][((y >> 9) & 56) + ((x >> 12) & 7)];

}


bool Level::checkSpikes (fixed x, fixed y) {

	// Anything off the edge of the map is not spikes
	if ((x < 0) || (y < 0) || (x > (LW * TW << 10)) || (y > (LH * TH << 10)))
		return false;

	// Event 126 is spikes
	if (grid[y >> 15][x >> 15].event != 126) return false;

	// Check the mask in the tile in question
	return mask[grid[y >> 15][x >> 15].tile][((y >> 9) & 56) + ((x >> 12) & 7)];

}


int Level::playFrame () {

	Bullet *nextBullet;
	Event *nextEvent;
	int x, y;


	// Search for active events
	for (y = (viewY >> 15) - 5; y < (((viewY >> 10) + viewH) >> 5) + 5; y++) {

		for (x = (viewX >> 15) - 5; x < (((viewX >> 10) + viewW) >> 5) + 5; x++)
			{

			if ((x >= 0) && (y >= 0) && (x < LW) && (y < LH) &&
				grid[y][x].event && (grid[y][x].event < 121)) {

				nextEvent = firstEvent;

				while (nextEvent) {

					// If the event has been found, stop searching
					if (nextEvent->isFrom(x, y)) break;

					nextEvent = nextEvent->getNext();

				}

				// If the event wasn't found, create it
				if (!nextEvent) firstEvent = new Event(x, y, firstEvent);

			}

		}

	}


	// Determine the players' trajectories
	for (x = 0; x < nPlayers; x++) players[x].control(ticks);


	// Process active events

	pathNode = (ticks >> 5) % pathLength;

	if (firstEvent) {

		if (firstEvent->playFrame(ticks)) {

			nextEvent = firstEvent->getNext();
			delete firstEvent;
			firstEvent = nextEvent;

		}

	}


	// Process bullets

	if (firstBullet) {

		if (firstBullet->playFrame(ticks)) {

			nextBullet = firstBullet->getNext();
			delete firstBullet;
			firstBullet = nextBullet;

		}

	}


	// Apply as much of those trajectories as possible, without going into the
	// scenery
	for (x = 0; x < nPlayers; x++) players[x].move(ticks);




	// Check if time has run out
	if ((ticks > endTime) && (difficulty >= 2)) {

		for (x = 0; x < nPlayers; x++) {

			if (players[x].getEnergy()) players[x].kill(endTime);

		}

	}

	// Calculate viewport
	if (game && winTime && (ticks > winTime)) game->view();
	else localPlayer->view(ticks);

	// Ensure the new viewport is within the level
	if (viewX < 0) viewX = 0;
	if ((viewX >> 10) + viewW >= LW * TW) viewX = ((LW * TW) - viewW) << 10;
	if (viewY < 0) viewY = 0;
	if ((viewY >> 10) + viewH >= LH * TH)	viewY = ((LH * TH) - viewH) << 10;


	return E_NONE;

}



void Level::draw () {

	GridElement *ge;
	Event *currentEvent;
	Bullet *currentBullet;
	SDL_Rect src, dst;
	int vX, vY;
	int x, y, bgScale;


	// Set tile drawing dimensions
	src.w = TW;
	src.h = TH;
	src.x = 0;


	// Use the local player's viewport
	dst.x = 0;
	dst.y = 0;
	vX = viewX >> 10;
	vY = viewY >> 10;
	dst.w = viewW;
	dst.h = viewH;
	SDL_SetClipRect(screen, &dst);


	if ((viewW < screenW) || (viewH < screenH)) SDL_FillRect(screen, NULL, 15);


	// If there is a sky, draw it
	if (sky) {

		// Background scale
		if (screenW > 320) bgScale = ((screenH - 1) / 100) + 1;
		else bgScale = ((screenH - 34) / 100) + 1;

		dst.x = 0;
		dst.w = screenW;
		dst.h = bgScale;

		for (y = 0; y < viewH; y += bgScale) {

			dst.y = y;
			SDL_FillRect(screen, &dst, 156 + (y / bgScale));

		}

		// Show sun / moon / etc.
		if (skyOrb) {

			dst.x = (viewW * 4) / 5;
			dst.y = (viewH * 3) / 25;
			src.y = skyOrb << 5;
			SDL_BlitSurface(tileSet, &src, screen, &dst);

		}

	} else {

		// If there is no sky, draw a blank background
		// This is only very occasionally actually visible
		SDL_FillRect(screen, NULL, 127);

	}



	// Show background tiles

	for (y = 0; y <= ((viewH - 1) >> 5) + 1; y++) {

		for (x = 0; x <= ((viewW - 1) >> 5) + 1; x++) {

			// Get the grid element from the given coordinates
			ge = grid[y + (vY >> 5)] + x + (vX >> 5);

			// If this tile uses a black background, draw it
			if (ge->bg) {

				dst.x = (x << 5) - (vX & 31);
				dst.y = (y << 5) - (vY & 31);
				dst.w = dst.h = TW;
				SDL_FillRect(screen, &dst, BLACK);

			}

			// If this is not a foreground tile, draw it
			if ((eventSet[ge->event][E_BEHAVIOUR] != 38) &&
			    ((ge->event < 124) || (ge->event > 125))  ) {

				dst.x = (x << 5) - (vX & 31);
				dst.y = (y << 5) - (vY & 31);
				src.y = ge->tile << 5;
				SDL_BlitSurface(tileSet, &src, screen, &dst);

			}

		}

	}


	// Show active events
	currentEvent = firstEvent;

	while (currentEvent) {

		currentEvent->draw(ticks);
		currentEvent = currentEvent->getNext();

	}


	// Show the players

	for (x = 0; x < nPlayers; x++) players[x].draw(ticks);


	// Show bullets
	currentBullet = firstBullet;

	while (currentBullet) {

		currentBullet->draw();
		currentBullet = currentBullet->getNext();

	}



	// Show foreground tiles

	for (y = 0; y <= ((viewH - 1) >> 5) + 1; y++) {

		for (x = 0; x <= ((viewW - 1) >> 5) + 1; x++) {

			// Get the grid element from the given coordinates
			ge = grid[y + (vY >> 5)] + x + (vX >> 5);

			// If this is an "animated" foreground tile, draw it
			if (ge->event == 123) {

				dst.x = (x << 5) - (vX & 31);
				dst.y = (y << 5) - (vY & 31);
				if (ticks & 64) src.y = eventSet[ge->event][E_YAXIS] << 5;
				else src.y = eventSet[ge->event][E_MULTIPURPOSE] << 5;
				SDL_BlitSurface(tileSet, &src, screen, &dst);

			}

			// If this is a foreground tile, draw it
			if ((ge->event == 124) || (ge->event == 125) ||
				(eventSet[ge->event][E_BEHAVIOUR] == 38)   ) {

				dst.x = (x << 5) - (vX & 31);
				dst.y = (y << 5) - (vY & 31);
				src.y = ge->tile << 5;
				SDL_BlitSurface(tileSet, &src, screen, &dst);

			}

		}

	}

	// Uncomment the following for a line showing the water level
/*	dst.x = 0;
	dst.y = (getWaterLevel(ticks) - viewY) >> 10;
	dst.w = screenW;
	dst.h = 2;
	SDL_FillRect(screen, &dst, 24); */


	SDL_SetClipRect(screen, NULL);


	// The panel
	// Design decision: When the width of the player's viewport is greater than
	// 320, the panel will not fill up the whole space. I decided that as the
	// game used the latin alphabet, and the latin alphabet is read from the
	// left, then the panel should appear to the left. Another option would have
	// been to have it in the centre, but this would obscure Jazz when he was at
	// the bottom of the level. As it is, Jazz is still obscured at viewport
	// widths between 321 and 672. A new approach may be needed, e.g. splitting
	// the panel down the middle.

	// Show panel

	// Change the ammo type display on the panel
	dst.x = 250;
	dst.y = 2;
	SDL_BlitSurface(panelAmmo[localPlayer->getAmmo(false) + 1], NULL, panel,
		&dst);

	dst.x = 0;
	dst.y = screenH - 33;
	SDL_BlitSurface(panel, NULL, screen, &dst);
	dst.y += 32;
	dst.w = 320;
	dst.h = 1;
	SDL_FillRect(screen, &dst, BLACK);


	// Show panel data

	// Show score
	panelSmallFont->showNumber(localPlayer->getScore(), 84, screenH - 27);

	// Show time remaining
	if (endTime > ticks) x = endTime - ticks;
	else x = 0;
	y = x / (60 * 1000);
	panelSmallFont->showNumber(y, 116, screenH - 27);
	x -= (y * 60 * 1000);
	y = x / 1000;
	panelSmallFont->showNumber(y, 136, screenH - 27);
	x -= (y * 1000);
	y = x / 100;
	panelSmallFont->showNumber(y, 148, screenH - 27);

	// Show lives
	panelSmallFont->showNumber(localPlayer->getLives(), 124, screenH - 13);

	// Show planet number

	
	if (worldNum <= 41) // Main game levels
		panelSmallFont->showNumber((worldNum % 3) + 1, 184, screenH - 13);
	else if ((worldNum >= 50) && (worldNum <= 52)) // Christmas levels
		panelSmallFont->showNumber(worldNum - 49, 184, screenH - 13);
	else panelSmallFont->showNumber(worldNum, 184, screenH - 13);

	// Show level number
	panelSmallFont->showNumber(levelNum + 1, 196, screenH - 13);

	// Show ammo
	if (localPlayer->getAmmo(false) == -1)
		panelSmallFont->showString(":;", 224, screenH - 13);
	else panelSmallFont->showNumber(localPlayer->getAmmo(true), 244,
		screenH - 13);


	// Draw the health bar

	dst.x = 20;
	dst.y = screenH - 13;
	dst.h = 7;
	x = localPlayer->getEnergyBar();

	if (x > F1) {

		dst.w = (x >> 10) - 1;

		x = localPlayer->getEnergy();

		// Choose energy bar colour
		if (x == 4) x = 24;
		else if (x == 3) x = 17;
		else if (x == 2) x = 80;
		else if (x <= 1) x = 32 + (((ticks / 75) * 4) & 15);

		// Draw energy bar
		SDL_FillRect(screen, &dst, x);

		dst.x += dst.w;
		dst.w = 64 - dst.w;

	} else dst.w = 64;


	// Fill in remaining energy bar space with black
	SDL_FillRect(screen, &dst, BLACK);


	return;

}


void Level::setNext (int nextLevel, int nextWorld) {

	unsigned char buffer[MTL_L_PROP];

	nextLevelNum = nextLevel;
	nextWorldNum = nextWorld;

	if (gameMode != M_SINGLE) {

		buffer[0] = MTL_L_PROP;
		buffer[1] = MT_L_PROP;
		buffer[2] = 0; // set next level
		buffer[3] = nextLevel;
		buffer[4] = nextWorld;

		game->send(buffer);

	}

	return;

}


void Level::setTile (unsigned char gridX, unsigned char gridY,
	unsigned char tile) {

	unsigned char buffer[MTL_L_GRID];

	grid[gridY][gridX].tile = tile;

	if (gameMode != M_SINGLE) {

		buffer[0] = MTL_L_GRID;
		buffer[1] = MT_L_GRID;
		buffer[2] = gridX;
		buffer[3] = gridY;
		buffer[4] = 0; // tile variable
		buffer[5] = tile;

		game->send(buffer);

	}

	return;

}


signed char * Level::getEvent (unsigned char gridX, unsigned char gridY) {

	int event = grid[gridY][gridX].event;

	if (event) return eventSet[grid[gridY][gridX].event];

	return NULL;

}


unsigned char Level::getEventHits (unsigned char gridX, unsigned char gridY) {

	return grid[gridY][gridX].hits;

}


int Level::getEventTime (unsigned char gridX, unsigned char gridY) {

	return grid[gridY][gridX].time;

}


void Level::clearEvent (unsigned char gridX, unsigned char gridY) {

	unsigned char buffer[MTL_L_GRID];

	grid[gridY][gridX].event = 0;

	if (gameMode != M_SINGLE) {

		buffer[0] = MTL_L_GRID;
		buffer[1] = MT_L_GRID;
		buffer[2] = gridX;
		buffer[3] = gridY;
		buffer[4] = 2; // event variable
		buffer[5] = 0;

		game->send(buffer);

	}

	return;

}


bool Level::hitEvent (unsigned char gridX, unsigned char gridY, bool TNT) {

	unsigned char buffer[MTL_L_GRID];
	int hitsToKill;

	if (TNT) grid[gridY][gridX].hits = 254;
	else grid[gridY][gridX].hits++;

	if (gameMode != M_SINGLE) {

		buffer[0] = MTL_L_GRID;
		buffer[1] = MT_L_GRID;
		buffer[2] = gridX;
		buffer[3] = gridY;
		buffer[4] = 3; // hits variable
		buffer[5] = grid[gridY][gridX].hits;

		game->send(buffer);

	}

	hitsToKill = eventSet[grid[gridY][gridX].event][E_HITSTOKILL];

	if (hitsToKill && (grid[gridY][gridX].hits >= hitsToKill)) return true;

	return false;

}


void Level::setEventTime (unsigned char gridX, unsigned char gridY, int time) {

	grid[gridY][gridX].time = time;

	return;

}


signed char * Level::getBullet (unsigned char bullet) {

	return bulletSet[bullet];

}


Sprite * Level::getSprite (unsigned char sprite) {

	return spriteSet + sprite;

}


Anim * Level::getAnim (unsigned char anim) {

	return animSet + anim;

}


Sprite * Level::getFrame (unsigned char anim, unsigned char frame) {

	return animSet[anim].sprites + (frame % animSet[anim].frames);

}


void Level::addTimer () {

	unsigned char buffer[MTL_L_PROP];

	endTime += 2 * 60 * 1000; // 2 minutes. Is this right?

	if (gameMode != M_SINGLE) {

		buffer[0] = MTL_L_PROP;
		buffer[1] = MT_L_PROP;
		buffer[2] = 2; // add timer
		buffer[3] = 0;
		buffer[4] = 0; // Don't really matter

		game->send(buffer);

	}

	return;

}


void Level::setWaterLevel (unsigned char gridY) {

	unsigned char buffer[MTL_L_PROP];

	waterLevel = gridY << 15;

	if (gameMode != M_SINGLE) {

		buffer[0] = MTL_L_PROP;
		buffer[1] = MT_L_PROP;
		buffer[2] = 1; // set water level
		buffer[3] = gridY;
		buffer[4] = 0; // Doesn't really matter

		game->send(buffer);

	}

	return;

}

fixed Level::getWaterLevel (int phase) {

	if (phase & 1024) return waterLevel - ((phase & 1023) * 32);
	return waterLevel - ((1024 - (phase & 1023)) * 32);

}


void Level::win () {

	unsigned char buffer[MTL_L_WON];

	winTime = ticks;

	if (gameMode != M_SINGLE) {

		buffer[0] = MTL_L_WON;
		buffer[1] = MT_L_WON;
		game->send(buffer);

	}

	return;

}


Scene * Level::createScene () {

	return new Scene(sceneFile);

}


void Level::receive (unsigned char *buffer) {

	// Interpret data received from client/server

	switch (buffer[1]) {

		case MT_L_PROP:

			if (buffer[2] == 0) {

				nextLevelNum = buffer[3];
				nextWorldNum = buffer[4];

			} else if (buffer[2] == 1) {

				waterLevel = buffer[3] << 15;

			} else if (buffer[2] == 2) {

				endTime += 2 * 60 * 1000; // 2 minutes. Is this right?

			}

			break;

		case MT_L_GRID:

			if (buffer[4] == 0) grid[buffer[3]][buffer[2]].tile = buffer[5];
			else if (buffer[4] == 2)
				grid[buffer[3]][buffer[2]].event = buffer[5];
			else if (buffer[4] == 3)
				grid[buffer[3]][buffer[2]].hits = buffer[5];

			break;

		case MT_L_WON:

			winTime = ticks;

			break;

	}

	return;

}


int Level::run () {

	char *options[5] = {"continue game", "save game", "load game",
		"setup options", "quit game"};
	PaletteEffect *levelPE;
	char *string;
	SDL_Rect dst;
	float smoothfps;
	bool paused, pmenu;
	int stats, option;
	int tickOffset, prevTicks;
 	int perfect, timeBonus;
 	int count;


	// Arbitrary initial value
	smoothfps = 50.0f;

	tickOffset = SDL_GetTicks();
	ticks = -10;

	pmenu = paused = false;
	option = 0;
	stats = S_NONE;

	timeBonus = -1;

	while (true) {

		// Do general processing
		if (loop() == E_QUIT) return E_QUIT;

		if (controls[C_ESCAPE].state) {

			releaseControl(C_ESCAPE);

			if (gameMode == M_SINGLE) paused = !paused;
			pmenu = !pmenu;
			option = 0;

		}

		if ((controls[C_PAUSE].state) &&
			(gameMode == M_SINGLE)) {

			releaseControl(C_PAUSE);

			paused = !paused;

		}

		if (controls[C_STATS].state) {

			releaseControl(C_STATS);

			if (gameMode == M_SINGLE) stats ^= S_SCREEN;
			else stats = (stats + 1) & 3;

		}

		if (pmenu) {

			// Deal with menu controls

			if (controls[C_UP].state) {

				releaseControl(C_UP);

				option = (option + 4) % 5;

			}

			if (controls[C_DOWN].state) {

				releaseControl(C_DOWN);

				option = (option + 1) % 5;

			}

			if (controls[C_ENTER].state) {

				releaseControl(C_ENTER);

				switch (option) {

					case 0: // Continue

						paused = pmenu = false;

						break;

					case 1: // Save

						break;

					case 2: // Load

						break;

					case 3: // Setup

						if (gameMode == M_SINGLE) {

							// Don't want palette effects in setup menu
							levelPE = firstPE;
							firstPE = NULL;

							if (menu->setup() == E_QUIT) return E_QUIT;

							// Restore level palette
							usePalette(palette);

							// Restore palette effects
							firstPE = levelPE;

						}

						break;

					case 4: // Quit game

						return E_NONE;

				}

			}

		}


		// Calculate smoothed fps
		smoothfps = smoothfps + 1 - (smoothfps * ((float)mspf) / 1000.0f);
		/* This equation is a simplified version of
		(fps * c) + (smoothfps * (1 - c))
		where c = (1 / fps)
		and fps = 1000 / mspf
		In other words, the response of smoothFPS to changes in FPS
		decreases as the framerate increases 
		The following version is for c = (1 / smoothfps)
		*/
		// smoothfps = (fps / smoothfps) + smoothfps - 1;

		// Ignore outlandish values
		if (smoothfps > 9999) smoothfps = 9999;
		if (smoothfps < 1) smoothfps = 1;


		// Number of ticks of gameplay since the level started

		prevTicks = ticks;
		ticks = SDL_GetTicks() - tickOffset;

		if (paused) {

			tickOffset += ticks - prevTicks;
			ticks = prevTicks;

		} else if (ticks > prevTicks + 100) {

			tickOffset += ticks - (prevTicks + 100);
			ticks = prevTicks + 100;

		}


		// Check if level has been won
		if (game && winTime && (ticks > winTime + T_WON)) {

			if (nextLevelNum == 99) count = game->setLevel(NULL);
			else {

				string = new char[11];
				sprintf(string, "level%1i.%03i", nextLevelNum, nextWorldNum);
				count = game->setLevel(string);
				delete[] string;

			}

			if (count < 0) return count;

			return WON;

		}


		// Process frame-by-frame activity

		if (!paused) {

			// Apply controls to local player
			for (count = 0; count < PCONTROLS; count++)
				localPlayer->setControl(count, controls[count].state);

			count = playFrame();

			if (count < 0) return count;


			// Handle player reactions
			for (count = 0; count < nPlayers; count++) {

				if (players[count].reacted(ticks) == PR_KILLED) {

					if (gameMode == M_SINGLE) return LOST;

					players[count].reset();
					players[count].setPosition(checkX << 15, checkY << 15);

				}

			}

		}


		// Draw the graphics

		draw();


		// If paused, draw "PAUSE"
		if (paused && !pmenu)
			fontmn1->showString("PAUSE", (screenW >> 1) - 44, 32);


		// If this is a competitive game, draw the score
		if ((gameMode != M_SINGLE) && (gameMode != M_COOP))
			fontmn1->showNumber(localPlayer->teamScore, 64, 4);


		// Draw player list

		if (stats & S_PLAYERS) {

			dst.x = (viewW >> 1) - 32;
			dst.y = 11;
			dst.w = 96;

			for (count = 0; count < nPlayers; count++)
				if ((strlen(players[count].getName()) * 8) + 57 > dst.w)
					dst.w = (strlen(players[count].getName()) * 8) + 57;

			dst.h = (nPlayers * 12) + 1;
			SDL_FillRect(screen, &dst, BLACK);

			for (count = 0; count < nPlayers; count++) {

				panelBigFont->showNumber(count + 1, dst.x + 24,
					14 + (count * 12));
				panelBigFont->showString(players[count].getName(), dst.x + 32,
					14 + (count * 12));
				panelBigFont->showNumber(players[count].teamScore,
					dst.x + dst.w - 8, 14 + (count * 12));

			}

		}


		// Draw graphics statistics

		if (stats & S_SCREEN) {

			dst.x = viewW - 84;
			dst.y = 11;
			dst.w = 80;
			dst.h = 25;
			SDL_FillRect(screen, &dst, BLACK);

			panelBigFont->showNumber(screenW, viewW - 52, 14);
			panelBigFont->showString("x", viewW - 48, 14);
			panelBigFont->showNumber(screenH, viewW - 12, 14);
			panelBigFont->showString("fps", viewW - 76, 26);
			panelBigFont->showNumber((int)smoothfps, viewW - 12, 26);

		}


		// If the level has been won, draw play statistics & bonuses

		if (winTime && (ticks > winTime)) {

			if (timeBonus == -1)
				timeBonus = ((endTime - winTime) / 60000) * 100;
			else {

				count = mspf / 5;
				if (!count) count = 1;

				if (timeBonus - count >= 0) {

					localPlayer->addScore(count);
					timeBonus -= count;

				} else {

					localPlayer->addScore(timeBonus);
					timeBonus = 0;

				}

			}

			fontmn1->showString("TIME", (screenW >> 1) - 152,
				(screenH >> 1) - 60);
			fontmn1->showNumber(timeBonus, (screenW >> 1) + 124,
				(screenH >> 1) - 60);

			fontmn1->showString("ENEMIES", (screenW >> 1) - 152,
				(screenH >> 1) - 40);

			if (enemies)
				fontmn1->showNumber((localPlayer->getEnemies() * 100) / enemies,
					(screenW >> 1) + 124, (screenH >> 1) - 40);
			else
				fontmn1->showNumber(0, (screenW >> 1) + 124,
					(screenH >> 1) - 40);

			fontmn1->showString("ITEMS", (screenW >> 1) - 152,
				(screenH >> 1) - 20);

			if (items)
				fontmn1->showNumber((localPlayer->getItems() * 100) / items,
					(screenW >> 1) + 124, (screenH >> 1) - 20);
			else
				fontmn1->showNumber(0, (screenW >> 1) + 124,
					(screenH >> 1) - 20);

			if ((localPlayer->getEnemies() == enemies) &&
				(localPlayer->getItems() == items)) perfect = 100;
			else perfect = 0;

			fontmn1->showString("PERFECT", (screenW >> 1) - 152, screenH >> 1);
			fontmn1->showNumber(perfect, (screenW >> 1) + 124, screenH >> 1);

			fontmn1->showString("SCORE", (screenW >> 1) - 152,
				(screenH >> 1) + 40);
			fontmn1->showNumber(localPlayer->getScore(), (screenW >> 1) + 124,
				(screenH >> 1) + 40);

		}


		if (pmenu) {

			// Draw the menu

			dst.x = (screenW >> 2) - 8;
			dst.y = (screenH >> 1) - 46;
			dst.w = 144;
			dst.h = 92;
			SDL_FillRect(screen, &dst, BLACK);

			for (count = 0; count < 5; count++) {

				if (count == option) fontmn2->mapPalette(240, 8, 47, -16);
				else fontmn2->mapPalette(240, 8, 15, -16);

				fontmn2->showString(options[count], screenW >> 2,
					(screenH >> 1) + (count << 4) - 38);

			}

			fontmn2->restorePalette();

		}


		// Networking

		if (gameMode != M_SINGLE) {

			count = game->playFrame(ticks);

			switch (count) {

				case E_UNUSED:

					return E_NONE;

				case E_NONE:

					break;

				default:

					return count;

			}

		}

	}

	return E_NONE;

}


int DemoLevel::run () {

	SDL_Rect dst;
	float smoothfps;
	int stats;
	int tickOffset, prevTicks;
	int timeBonus;
	unsigned char macroPoint;
	int ret;


	// Arbitrary initial value
	smoothfps = 50.0f;

	tickOffset = SDL_GetTicks();
	ticks = -10;

	stats = S_NONE;

	timeBonus = -1;

	while (true) {

		// Do general processing
		if (loop() == E_QUIT) return E_QUIT;

		if (controls[C_ESCAPE].state) {

			releaseControl(C_ESCAPE);

			return E_NONE;

		}

		if (controls[C_STATS].state) {

			releaseControl(C_STATS);

			stats ^= S_SCREEN;

		}


		// Calculate smoothed fps
		smoothfps = smoothfps + 1 - (smoothfps * ((float)mspf) / 1000.0f);
		/* This equation is a simplified version of
		(fps * c) + (smoothfps * (1 - c))
		where c = (1 / fps)
		and fps = 1000 / mspf
		In other words, the response of smoothFPS to changes in FPS
		decreases as the framerate increases 
		The following version is for c = (1 / smoothfps)
		*/
		// smoothfps = (fps / smoothfps) + smoothfps - 1;

		// Ignore outlandish values
		if (smoothfps > 9999) smoothfps = 9999;
		if (smoothfps < 1) smoothfps = 1;


		// Number of ticks of gameplay since the level started

		prevTicks = ticks;
		ticks = SDL_GetTicks() - tickOffset;

		if (ticks > prevTicks + 100) {

			tickOffset += ticks - (prevTicks + 100);
			ticks = prevTicks + 100;

		}



		// Use macro

		macroPoint = macro[(ticks / 90) % 1024];

		if (macroPoint & 128) return E_NONE;

		if (macroPoint & 1) {

			localPlayer->setControl(C_LEFT, false);
			localPlayer->setControl(C_RIGHT, false);
			localPlayer->setControl(C_UP, !(macroPoint & 4));

		} else {

			localPlayer->setControl(C_LEFT, !(macroPoint & 2));
			localPlayer->setControl(C_RIGHT, macroPoint & 2);
			localPlayer->setControl(C_UP, false);

		}

		localPlayer->setControl(C_DOWN, macroPoint & 8);
		localPlayer->setControl(C_FIRE, macroPoint & 16);
		localPlayer->setControl(C_CHANGE, macroPoint & 32);
		localPlayer->setControl(C_JUMP, macroPoint & 64);



		// Check if level has been won
		if (winTime && (ticks > winTime + T_WON)) return WON;


		// Process frame-by-frame activity

		ret = playFrame();

		if (ret < 0) return ret;


		// Handle player reactions
		if (localPlayer->reacted(ticks) == PR_KILLED) return LOST;


		// Draw the graphics

		draw();


		fontmn1->showString("DEMO", (screenW >> 1) - 36, 32);


		// Draw graphics statistics

		if (stats & S_SCREEN) {

			dst.x = 236;
			dst.y = 9;
			dst.w = 80;
			dst.h = 32;
			SDL_FillRect(screen, &dst, BLACK);

			panelBigFont->showNumber(screenW, 268, 15);
			panelBigFont->showString("x", 272, 15);
			panelBigFont->showNumber(screenH, 308, 15);
			panelBigFont->showString("fps", 244, 27);
			panelBigFont->showNumber((int)smoothfps, 308, 27);

		}

	}

	return E_NONE;

}


