#include "StatsTracker.h"

StatsTracker::StatsTracker(std::vector<unsigned int> team_ids) {
	for (long unsigned int i = 0; i < team_ids.size(); i++) {
		_team_stats[i] = TeamStats{};
		_team_stats[i].movement = 0;
		_team_stats[i].units_died = 0;
		_team_stats[i].units_created = 0;
		_team_stats[i].units_killed = 0;
		_team_stats[i].total_damage = 0;
		_team_stats[i].resources_mined = 0;
		_team_stats[i].money_gained = 0;
		_team_stats[i].money_spent = 0;
		_team_stats[i].walls_built = 0;
		_team_stats[i].walls_destroyed = 0;
		_team_stats[i].cores_destroyed = 0;
		_team_stats[i].damage_on_units = 0;
		_team_stats[i].damage_on_cores = 0;
	}

}

StatsTracker::~StatsTracker() {
	// nothing to do here :)
}

void StatsTracker::incrementRightStats(unsigned int team_id, unsigned int stat, unsigned int amount) {
	team_id--;
	if (team_id >= _team_stats.size()) {
		std::cerr << "Invalid team id" << std::endl;
		return;
	}
	switch (stat) {
		case MOVEMENT:
			_team_stats[team_id].movement += amount;
			break;
		case UNITS_DIED:
			_team_stats[team_id].units_died += amount;
			break;
		case UNITS_CREATED:
			_team_stats[team_id].units_created += amount;
			break;
		case UNITS_KILLED:
			_team_stats[team_id].units_killed += amount;
			break;
		case TOTAL_DAMAGE:
			_team_stats[team_id].total_damage += amount;
			break;
		case RECOURCES_MINED:
			_team_stats[team_id].resources_mined += amount;
			break;
		case MONEY_GAINED:
			_team_stats[team_id].money_gained += amount;
			break;
		case MONEY_SPENT:
			_team_stats[team_id].money_spent += amount;
			break;
		case WALLS_BUID:
			_team_stats[team_id].walls_built += amount;
			break;
		case WALLS_DESTROYED:
			_team_stats[team_id].walls_destroyed += amount;
			break;
		case CORES_DESTROYED:
			_team_stats[team_id].cores_destroyed += amount;
			break;
		case DAMAGE_ON_UNITS:
			_team_stats[team_id].damage_on_units += amount;
			break;
		case DAMAGE_ON_CORES:
			_team_stats[team_id].damage_on_cores += amount;
			break;
		default:
			std::cerr << "Invalid stat" << std::endl;
			break;
	}
}


void StatsTracker::printStats() {
	std::cout << "Stats:" << std::endl;
	for (long unsigned int i = 0; i < _team_stats.size(); i++) {
		std::cout << "Team " << i << ":" << std::endl;
		std::cout << "  Movement: " << _team_stats[i].movement << std::endl;
		std::cout << "  Units Died: " << _team_stats[i].units_died << std::endl;
		std::cout << "  Units Built: " << _team_stats[i].units_created << std::endl;
		std::cout << "  Units Killed: " << _team_stats[i].units_killed << std::endl;
		std::cout << "  Total Damage: " << _team_stats[i].total_damage << std::endl;
		std::cout << "  Damage on Units: " << _team_stats[i].damage_on_units << std::endl;
		std::cout << "  Damage on Cores: " << _team_stats[i].damage_on_cores << std::endl;
		std::cout << "  Resources Mined: " << _team_stats[i].resources_mined << std::endl;
		std::cout << "  Money Gained: " << _team_stats[i].money_gained << std::endl;
		std::cout << "  Money Spent: " << _team_stats[i].money_spent << std::endl;
		std::cout << "  Walls Built: " << _team_stats[i].walls_built << std::endl;
		std::cout << "  Walls Destroyed: " << _team_stats[i].walls_destroyed << std::endl;
		std::cout << "  Cores Destroyed: " << _team_stats[i].cores_destroyed << std::endl;
	}
}

json StatsTracker::getStats() {
	json stats;
	for (long unsigned int i = 0; i < _team_stats.size(); i++) {
		json team;
		team["movement"] = _team_stats[i].movement;
		team["units_died"] = _team_stats[i].units_died;
		team["units_build"] = _team_stats[i].units_created;
		team["units_killed"] = _team_stats[i].units_killed;
		team["total_damage"] = _team_stats[i].total_damage;
		team["damage_on_units"] = _team_stats[i].damage_on_units;
		team["damage_on_cores"] = _team_stats[i].damage_on_cores;
		team["resources_mined"] = _team_stats[i].resources_mined;
		team["money_gained"] = _team_stats[i].money_gained;
		team["money_spent"] = _team_stats[i].money_spent;
		team["walls_built"] = _team_stats[i].walls_built;
		team["walls_destroyed"] = _team_stats[i].walls_destroyed;
		stats.push_back(team);
	}
	return stats;
}
