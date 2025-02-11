#ifndef GAME_H
#define GAME_H

#include <vector>
#include <chrono>
#include <iostream>

#include "Config.h"
#include "Core.h"
#include "Unit.h"
#include "Resource.h"
#include "Wall.h"
#include "Bridge.h"
#include "Action.h"
#include "Utils.h"

#include "json.hpp"
using json = nlohmann::ordered_json;

class Game
{
	public:
		Game(std::vector<unsigned int> team_ids);
		~Game();
		void addBridge(Bridge* bridge);

		void run();

		Core * getCore(unsigned int teamId);
		Object * getObject(unsigned int id);

		Object * getObjectAtPos(Position pos);

	private:
		void tick(unsigned long long tick);
		void sendState();
		void sendConfig();

		unsigned int teamCount_;
		unsigned int nextObjectId_;
		std::vector<std::unique_ptr<Object>> objects_;
		std::vector<Bridge*> bridges_;

		void visualizeGameState();
};

#endif // GAME_H
