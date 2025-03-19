#ifndef STATSTRACKER_H
#define STATSTRACKER_H

#include <vector>
#include <chrono>
#include <iostream>
#include <fstream>

#include "json.hpp"
using json = nlohmann::ordered_json;

// object for tracking stats of each team
struct TeamStats {
	unsigned int movement;
	unsigned int units_died;
	unsigned int units_built;
	unsigned int units_killed;
};

class StatsTracker {
public:
	StatsTracker(std::vector<unsigned int> team_ids);
	StatsTracker() = default;
	~StatsTracker();
	
	void incrementMovement(unsigned int team_id);
	void incrementUnitsDied(unsigned int team_id);
	void printStats();
	json getStats();
	std::vector<TeamStats> team_stats;
};



#endif // STATSTRACKER_H