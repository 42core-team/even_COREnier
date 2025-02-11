#ifndef WORLD_GENERATOR_H
#define WORLD_GENERATOR_H

#include <vector>
#include <memory>

#include "Object.h"
#include "Resource.h"
#include "Wall.h"

class Game;

class WorldGenerator
{
	public:
	WorldGenerator();
	virtual ~WorldGenerator();
	
	virtual void generateWorld(Game * game) = 0;
};

#include "Game.h"

#endif // WORLD_GENERATOR_H
