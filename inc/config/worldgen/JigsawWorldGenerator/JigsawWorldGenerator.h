#pragma once

#include "Config.h"
#include "MapTemplate.h"
#include "WorldGenerator.h"
#include "Logger.h"
#include "Object.h"
#include "Wall.h"
#include "Resource.h"
#include "Money.h"
#include "Board.h"
#include "Visualizer.h"

#include <vector>
#include <string>
#include <filesystem>
#include <queue>
#include <cstdlib>
#include <climits>
#include <algorithm>
#include <chrono>
#include <random>

struct Rectangle {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
};

class JigsawWorldGenerator : public WorldGenerator {
	public:
		JigsawWorldGenerator(unsigned int seed = 0);

		void generateWorld();

	private:
		std::vector<MapTemplate> templates_;

		std::default_random_engine eng_ = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());

		void loadTemplates();

		bool tryPlaceTemplate(const MapTemplate &temp, int posX, int posY, bool force);
		bool canPlaceTemplate(const MapTemplate &temp, int posX, int posY);

		void balanceObjectType(ObjectType type, int amount);
		void clearPathBetweenCores();
		void placeWalls();
		void mirrorWorld();
};
