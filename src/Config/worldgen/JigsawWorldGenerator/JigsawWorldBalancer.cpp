#include "JigsawWorldGenerator.h"

void JigsawWorldGenerator::balanceResourceAmount(Game* game)
{
	std::vector<size_t> resourceIndices;
	for (size_t i = 0; i < game->getObjects().size(); ++i)
	{
		bool canBeRemoved = true;
		if (game->getObjects()[i]->getType() != ObjectType::Resource)
			canBeRemoved = false;
		for (const auto &safeZone : coreWallRegions_)
		{
			if (game->getObjects()[i]->getPosition().x >= safeZone.x &&
				game->getObjects()[i]->getPosition().x < safeZone.x + safeZone.width &&
				game->getObjects()[i]->getPosition().y >= safeZone.y &&
				game->getObjects()[i]->getPosition().y < safeZone.y + safeZone.height)
			{
				canBeRemoved = false;
				break;
			}
		}

		if (canBeRemoved)
			resourceIndices.push_back(i);
	}
	int currentResources = resourceIndices.size();
	
	if (currentResources > expectedResourceCount_)
	{
		int removeCount = currentResources - expectedResourceCount_;

		std::shuffle(resourceIndices.begin(), resourceIndices.end(), eng_);

		std::vector<size_t> indicesToRemove(resourceIndices.begin(), resourceIndices.begin() + removeCount);
		std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<size_t>());

		for (size_t idx : indicesToRemove)
		{
			game->getObjects().erase(game->getObjects().begin() + idx);
		}
	}
	else if (currentResources < expectedResourceCount_)
	{
		int addCount = expectedResourceCount_ - currentResources;
		std::uniform_int_distribution<int> distX(0, Config::getInstance().width - 1);
		std::uniform_int_distribution<int> distY(0, Config::getInstance().height - 1);

		while (addCount > 0)
		{
			int x = distX(eng_);
			int y = distY(eng_);
			Position pos(x, y);

			if (game->getObjectAtPos(pos) == nullptr)
			{
				bool nearCore = false;
				for (const auto &safeZone : coreWallRegions_)
				{
					if (pos.x >= safeZone.x && pos.x < safeZone.x + safeZone.width &&
						pos.y >= safeZone.y && pos.y < safeZone.y + safeZone.height)
					{
						nearCore = true;
						break;
					}
				}
				if (nearCore)
					continue;

				game->getObjects().push_back(std::make_unique<Resource>(game->getNextObjectId(), pos));
				addCount--;
			}
		}
	}
}

static bool isWithinBounds(int x, int y, unsigned int width, unsigned int height)
{
	return (x >= 0 && x < (int)width && y >= 0 && y < (int)height);
}

// Dijkstra helper node
struct Node {
	int cost;
	Position pos;
	bool operator>(const Node &other) const {
		return cost > other.cost;
	}
};

// map of how many steps it takes to get to each position a given start position
std::vector<std::vector<int>> computeDistanceMap(Game* game, const Position& start)
{
	unsigned int width = Config::getInstance().width;
	unsigned int height = Config::getInstance().height;
	std::vector<std::vector<int>> dist(height, std::vector<int>(width, std::numeric_limits<int>::max()));
	
	double sumWallDamage = 0;
	double sumResourceDamage = 0;
	const auto &units = Config::getInstance().units;
	for (const auto &unit : units)
	{
		sumWallDamage += unit.damageWall;
		sumResourceDamage += unit.damageResource;
	}
	double avgWallDamage = (units.size() > 0) ? (sumWallDamage / units.size()) : 1.0;
	double avgResourceDamage = (units.size() > 0) ? (sumResourceDamage / units.size()) : 1.0;
	
	int wallCost = std::ceil((double)Config::getInstance().wallHp / avgWallDamage);
	int resourceCost = std::ceil((double)Config::getInstance().resourceHp / avgResourceDamage);
	
	std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
	dist[start.y][start.x] = 0;
	pq.push({0, start});
	
	std::vector<Position> directions = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
	
	while (!pq.empty())
	{
		Node node = pq.top();
		pq.pop();
		Position cur = node.pos;
		int curCost = node.cost;
		
		if (curCost > dist[cur.y][cur.x])
			continue;
		
		for (const auto &d : directions)
		{
			int nx = cur.x + d.x;
			int ny = cur.y + d.y;
			if (isWithinBounds(nx, ny, width, height)) 
			{
				Position neighbor(nx, ny);
				int stepCost = 1;
				
				auto obj = game->getObjectAtPos(neighbor);
				if (obj != nullptr)
				{
					if (obj->getType() == ObjectType::Wall)
						stepCost = wallCost;
					else if (obj->getType() == ObjectType::Resource)
						stepCost = resourceCost;
				}
				
				if (dist[ny][nx] > curCost + stepCost)
				{
					dist[ny][nx] = curCost + stepCost;
					pq.push({dist[ny][nx], neighbor});
				}
			}
		}
	}
	return dist;
}

// if you need to change anything about this, may god have mercy on your soul
void JigsawWorldGenerator::balanceResourcesBetweenPlayers(Game* game)
{
	const auto& cores = game->getCores();
	if (cores.empty())
		return; 

	unsigned int width = Config::getInstance().width;
	unsigned int height = Config::getInstance().height;

	// 1. Compute distance maps (BFS) for each core.
	std::vector<std::vector<std::vector<int>>> coreDistanceMaps;
	for (const auto &core : cores) {
		coreDistanceMaps.push_back(computeDistanceMap(game, core.getPosition()));
	}

	// 2. Assign each resource to the closest core by Manhattan distance.
	// Map: core index -> vector of resource indices (into game->getObjects())
	std::vector<std::vector<size_t>> coreResources(cores.size()); // vector 1: core, vector 2: resource, value: index in game->getObjects()
	for (size_t i = 0; i < game->getObjects().size(); ++i)
	{
		auto &obj = game->getObjects()[i];
		if (obj->getType() != ObjectType::Resource)
			continue;
		Position pos = obj->getPosition();
		int bestDijkstra = std::numeric_limits<int>::max();
		size_t bestCore = 0;
		for (size_t c = 0; c < cores.size(); ++c)
		{
			Position corePos = cores[c].getPosition();
			int dijkstra = coreDistanceMaps[c][pos.y][pos.x];
			if (dijkstra < bestDijkstra)
			{
				bestDijkstra = dijkstra;
				bestCore = c;
			}
		}
		coreResources[bestCore].push_back(i);
	}

	// 3. Build frequency maps for each core: distance -> count.
	// Also record for each resource its computed Dijkstra distance from its assigned core.
	std::vector<std::map<int, int>> coreFreq(cores.size());
	std::vector<int> resourceDistance(game->getObjects().size(), -1); // indexed by resource index

	for (size_t c = 0; c < cores.size(); ++c)
	{
		for (auto resIdx : coreResources[c])
		{
			Position pos = game->getObjects()[resIdx]->getPosition();
			int d = coreDistanceMaps[c][pos.y][pos.x];
			coreFreq[c][d]++;
			resourceDistance[resIdx] = d;
		}
	}

	// 4. Begin rebalancing until equilibrium: difference at any distance is <= 1.
	const int MAX_ITER = 1000;
	int iter = 0;
	while (iter < MAX_ITER)
	{
		bool balanced = true;
		// every distance level that appears in any core's frequency map
		std::set<int> allDistances;
		for (const auto &freqMap : coreFreq)
		{
			for (const auto &p : freqMap)
				allDistances.insert(p.first);
		}

		// Check imbalance at each distance level.
		for (int d : allDistances)
		{
			// determine max & mins for that distance level
			int minCount = std::numeric_limits<int>::max();
			int maxCount = -1;
			size_t coreMin = 0, coreMax = 0;
			for (size_t c = 0; c < cores.size(); ++c)
			{
				int count = (coreFreq[c].count(d) ? coreFreq[c].at(d) : 0);
				if (count < minCount)
				{
					minCount = count;
					coreMin = c;
				}
				if (count > maxCount)
				{
					maxCount = count;
					coreMax = c;
				}
			}

			// if the difference is 0 or 1, this distance level is balanced
			if (maxCount - minCount >= 1)
				continue;

			balanced = false;
			// Core 'coreMax' has an excess resource at distance d.
			// Let's choose a resource in coreMax with that distance.
			size_t resourceToMove = SIZE_MAX;
			for (auto resIdx : coreResources[coreMax])
			{
				if (resourceDistance[resIdx] == d)
				{
					resourceToMove = resIdx;
					break;
				}
			}
			if (resourceToMove == SIZE_MAX)
			{
				// what the heck
				Logger::Log(LogLevel::ERROR, "Resource balancing error: no resource found to move. This should not happen given common laws of logic.");
				continue;
			}

			// Remove this resource from coreMax's record:
			coreFreq[coreMax][d]--;
			// Erase resourceToMove from coreResources[coreMax]:
			auto it = std::find(coreResources[coreMax].begin(), coreResources[coreMax].end(), resourceToMove);
			if (it != coreResources[coreMax].end())
				coreResources[coreMax].erase(it);

			// TODO: make this not insert at bfs, but at dijkstra position of d instead
			// Now, we need to find a new position on coreMin's side.
			// Our aim: a cell with BFS distance == d from coreMin, and free of objects.
			// We scan the grid; one could optimize this by, say, storing candidates in advance.
			Position newPos(-1, -1);
			const auto &bfsMap = coreDistanceMaps[coreMin];
			bool found = false;
			for (int y = 0; y < (int)height && !found; ++y) {
				for (int x = 0; x < (int)width && !found; ++x) {
					// Check if the BFS distance matches:
					if (bfsMap[y][x] != d)
						continue;
					// Check if the cell is free:
					if (game->getObjectAtPos(Position(x, y)) != nullptr)
						continue;
					// Also, ensure that by Manhattan, this cell belongs to coreMin:
					Position pos(x, y);
					int bestManhattan = std::numeric_limits<int>::max();
					size_t bestCore = 0;
					for (size_t c = 0; c < cores.size(); ++c) {
						Position corePos = cores[c].getPosition();
						int manhattan = std::abs(pos.x - corePos.x) + std::abs(pos.y - corePos.y);
						if (manhattan < bestManhattan) {
							bestManhattan = manhattan;
							bestCore = c;
						}
					}
					if (bestCore == coreMin) {
						newPos = pos;
						found = true;
					}
				}
			}
			if (!found) {
				// If we cannot find a cell at exactly distance d, one might relax this constraint
				// (perhaps search for distance d+1 or d-1) but for brevity we simply skip.
				// A more robust solution would search in a small radius. 😉
				continue;
			}
			
			// Move the resource to the new position.
			game->getObjects()[resourceToMove]->setPosition(newPos);
			// Update the record for coreMin:
			coreFreq[coreMin][d]++;
			coreResources[coreMin].push_back(resourceToMove);
			resourceDistance[resourceToMove] = d;
			
			// One move done—break out of the loop to recalc imbalances.
			break;
		}
		if (balanced)
			break;
		++iter;
	}
	
	if (iter >= MAX_ITER) {
		Logger::Log(LogLevel::WARNING, "Fair balancing did not converge fully after " + std::to_string(MAX_ITER) + " iterations.");
	} else {
		Logger::Log(LogLevel::INFO, "Fair resource balancing complete in " + std::to_string(iter) + " iterations.");
	}
}
