
/*
 *
 * bird.cpp
 *
 * Created on the 1st of March 2009 from parts of events.cpp
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


#include "OpenJazz.h"


Bird::Bird (Player *rescuer, unsigned char gX, unsigned char gY) {

	player = rescuer;
	x = gX << 15;
	y = gY << 15;
	dx = 0;
	dy = 0;
	fleeing = false;
	fireTime = 0;

	return;

}


Bird::~Bird () {

	return;

}


void Bird::reset () {

	x = player->getX();
	y = player->getY() - F64;
	fireTime = 0;

	return;

}


Player * Bird::getPlayer () {

	return player;

}


void Bird::hit () {

	fleeing = true;

	return;

}


fixed Bird::getX () {

	return x;

}


fixed Bird::getY () {

	return y;

}


bool Bird::playFrame (int ticks) {

	if (fleeing) {

		// Trajectory for flying away
		dx = F80;
		dy = -F80;

		// If the bird has flown off-screen, remove it
		if (y < viewY - F160) return true;

	} else {

		// Trajectory for flying towards the player

		if ((x < player->getX() - F160) || (x > player->getX() + F160)) {

			dx = player->getX() - x;

		} else if (x < player->getX()) {

			if (dx < F160) dx += 400 * mspf;

		} else {

			if (dx > -F160) dx -= 400 * mspf;

		}

		if (y > level->getWaterLevel(ticks) - F24) {

			y = level->getWaterLevel(ticks) - F24;
			dy = 0;

		} else {

			if ((y < player->getY() - F100) || (y > player->getY() + F100)) {

				dy = (player->getY() - F64) - y;

			} else if (y < player->getY() - F64) {

				if (dy < F160) dy += 400 * mspf;

			} else {

				if (dy > -F160) dy -= 400 * mspf;

			}


		}


		// If there are enemies in the vicinity, generate bullets
		if (true && (ticks > fireTime)) {

			level->firstBullet =
				new Bullet(this, false, ticks, level->firstBullet);

			fireTime = ticks + T_BIRD_FIRE;

		}

	}

	// Apply trajectory
	x += (dx * mspf) >> 10;
	y += (dy * mspf) >> 10;

	return false;

}


void Bird::draw (int ticks) {

	SDL_Rect dst;
	int anim, frame;

	anim = (player->getFacing() || fleeing) ? BIRD_RIGHTANIM: BIRD_LEFTANIM;
	frame = ticks / 80;

	dst.x = (x >> 10) - (viewX >> 10) + level->getFrame(anim, frame)->x;

	dst.y = (y >> 10) - (viewY >> 10) + level->getFrame(anim, frame)->y;

	SDL_BlitSurface(level->getFrame(anim, frame)->pixels, NULL, screen, &dst);

	return;

}


