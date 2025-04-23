#ifndef STATSTRACKER_H
#define STATSTRACKER_H

#include <map>
#include <chrono>
#include <iostream>
#include <fstream>

#define MOVEMENT 0
#define UNITS_DIED 1
#define UNITS_CREATED 2
#define UNITS_KILLED 3
#define TOTAL_DAMAGE 4
#define RECOURCES_MINED 5
#define MONEY_GAINED 6
#define MONEY_SPENT 7
#define WALLS_BUID 8
#define WALLS_DESTROYED 9
#define CORES_DESTROYED 10
#define DAMAGE_ON_UNITS 11
#define DAMAGE_ON_CORES 12

#include "json.hpp"
using json = nlohmann::ordered_json;

// object for tracking stats of each team
struct TeamStats {
	unsigned int movement;
	unsigned int units_died;
	unsigned int units_created;
	unsigned int units_killed;
	unsigned int cores_destroyed;
	unsigned int total_damage;
	unsigned int damage_on_units;
	unsigned int damage_on_cores;
	unsigned int resources_mined;
	unsigned int money_gained;
	unsigned int money_spent;
	unsigned int walls_built;
	unsigned int walls_destroyed;
};

class StatsTracker {
public:
	StatsTracker(std::vector<unsigned int> team_ids);
	StatsTracker() = default;
	~StatsTracker();
	
	void incrementRightStats(unsigned int team_id, unsigned int stat, unsigned int amount);

	void incrementMovement(unsigned int team_id);
	void incrementUnitsDied(unsigned int team_id);
	void printStats();
	json getStats();
	std::map<int, TeamStats> _team_stats;
};



#endif // STATSTRACKER_H