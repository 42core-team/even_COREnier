#include "StatsTracker.h"

StatsTracker::StatsTracker(std::vector<unsigned int> team_ids) {
	team_stats.resize(team_ids.size());
	for (long unsigned int i = 0; i < team_ids.size(); i++) {
		team_stats[i].movement = 0;
		team_stats[i].units_died = 0;
		team_stats[i].units_built = 0;
		team_stats[i].units_killed = 0;
	}

}

StatsTracker::~StatsTracker() {
	// nothing to do here :)
}

void StatsTracker::incrementMovement(unsigned int team_id) {
	team_id--;
	// std::cout << "team id: " << team_id << std::endl;
	if (team_id >= team_stats.size()) {
		std::cerr << "Invalid team id" << std::endl;
		return;
	}
	team_stats[team_id].movement++;
}

void StatsTracker::incrementUnitsDied(unsigned int team_id) {
	team_id--;
	if (team_id >= team_stats.size()) {
		std::cerr << "Invalid team id" << std::endl;
		return;
	}
	team_stats[team_id].units_died++;
}

void StatsTracker::printStats() {
	std::cout << "Stats:" << std::endl;
	for (long unsigned int i = 0; i < team_stats.size(); i++) {
		std::cout << "Team " << i << ":" << std::endl;
		std::cout << "  Movement: " << team_stats[i].movement << std::endl;
		// std::cout << "  Units Died: " << team_stats[i].units_died << std::endl;
		// std::cout << "  Units Built: " << team_stats[i].units_built << std::endl;
		// std::cout << "  Units Killed: " << team_stats[i].units_killed << std::endl;
	}
}

json StatsTracker::getStats() {
	json stats;
	for (long unsigned int i = 0; i < team_stats.size(); i++) {
		json team;
		team["movement"] = team_stats[i].movement;
		team["units_died"] = team_stats[i].units_died;
		team["units_built"] = team_stats[i].units_built;
		team["units_killed"] = team_stats[i].units_killed;
		stats.push_back(team);
	}
	return stats;
}
