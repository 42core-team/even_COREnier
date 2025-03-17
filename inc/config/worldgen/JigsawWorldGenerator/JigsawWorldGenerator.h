#pragma once

#include "Game.h"
#include "Config.h"
#include "MapTemplate.h"

#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <random>

struct Rectangle {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
};

class JigsawWorldGenerator {
	public:
		JigsawWorldGenerator(unsigned int seed = 0);

		void generateWorld(Game* game);

	private:
		std::vector<MapTemplate> templates_;
		int minSpacing_ = 1;
		int expectedResourceCount_ = 20;

		std::vector<Rectangle> coreWallRegions_;

		std::default_random_engine eng_ = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());

		void loadTemplates();

		bool tryPlaceTemplate(Game* game, const MapTemplate &temp, int posX, int posY, bool force);
		bool canPlaceTemplate(Game* game, const MapTemplate &temp, int posX, int posY);

		void balanceResources(Game* game);
		void placeWalls(Game* game);

		void placeCoreWalls(Game* game);
};
