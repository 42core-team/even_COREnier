#include "Game.h"

#include <string>

void Game::visualizeGameState(unsigned long long tick)
{
	std::cout << "Tick: " << tick << "; Core: C / c; Units: W / w = Warrior; M / m = Miner; . = empty; R = Resource; X = Wall" << std::endl;

	unsigned int resourceCount = 0;
	unsigned int wallCount = 0;
	unsigned int coreCount = 0;
	unsigned int unitCount = 0;

	for (int y = 0; y < (int)Config::getInstance().height; y++)
	{
		for (int x = 0; x < (int)Config::getInstance().width; x++)
		{
			Position pos = Position(x, y);
			Object * obj = getObjectAtPos(pos);
			if (obj)
			{
				if (obj->getType() == ObjectType::Core)
				{
					Core * core = dynamic_cast<Core*>(obj);
					std::cout << "Cc"[core->getTeamId() % 2];
					coreCount++;
				}
				else if (obj->getType() == ObjectType::Unit)
				{
					Unit * unit = dynamic_cast<Unit*>(obj);
					std::string unitInd = "??";
					if (unit->getTypeId() == 0)
						unitInd = "Ww";
					else if (unit->getTypeId() == 1)
						unitInd = "Mm";
					else if (unit->getTypeId() == 2)
						unitInd = "Cc";
					else if (unit->getTypeId() == 3)
						unitInd = "Bb";
					else if (unit->getTypeId() == 4)
						unitInd = "Aa";
					std::cout << unitInd[unit->getTeamId() % 2];
					unitCount++;
				}
				else if (obj->getType() == ObjectType::Resource)
				{
					std::cout << "R";
					resourceCount++;
				}
				else if (obj->getType() == ObjectType::Wall)
				{
					std::cout << "X";
					wallCount++;
				}
			}
			else
			{
				std::cout << ".";
			}
		}
		std::cout << std::endl;
	}
	
	std::cout << "ResourceCount: " << resourceCount << std::endl;
	std::cout << "UnitCount: " << unitCount << std::endl;
	std::cout << "WallCount: " << wallCount << std::endl;
	std::cout << "CoreCount: " << coreCount << std::endl;

	std::cout << std::endl;
}
