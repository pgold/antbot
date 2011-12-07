#include "Bot.h"

#include <cassert>
#include <cstring>

#ifdef DEBUG
	#define TRACE(x) x
#else
	#define TRACE(x) 
#endif // DEBUG

#define m(i, j) (i*state.cols)+j
#define invdir(dir) ((dir+2)%4)
#define checkTimer() if(state.timer.getTime() > 900.0f) {state.bug << "Timer" << endl; return;}

using namespace std;

typedef std::vector<Location> locvec;

#define BOARD_SIZE 200

const int minColony = 5;
const int minColonyForSpartans = 30; // min colony size before trying spartans
const int numSpartans = 10; // max ammount to attack hill
const int minSpartans = 4; // min ammount to atack a hill 
const int minColonyForGuardians = 5; // min colony size before scheduling guardians

std::vector<std::vector<int> > path;
int dir[BOARD_SIZE][BOARD_SIZE];
int moved[BOARD_SIZE][BOARD_SIZE];

//constructor
Bot::Bot()
{
};

//plays a single game of Ants.
void Bot::playGame()
{
    //reads the game parameters and sets up
    cin >> state;
    state.setup();
    endTurn();

	mark = (int*) malloc(state.rows*state.cols*sizeof(int));
	pred = (int*) malloc(state.rows*state.cols*sizeof(int));
	explored = false;

#ifdef DEBUG
	assert(mark != NULL);
	assert(pred != NULL);
#endif /* DEBUG */

    //continues making moves while the game is not over
    while(cin >> state)
    {
        state.updateVisionInformation();
        makeMoves();
        endTurn();
    }

	free(mark);
	free(pred);
};

int foodDirection;
//makes the bots moves for the turn
void Bot::makeMoves()
{
    state.bug << "turn " << state.turn << ":" << endl;
    //state.bug << state << endl;

	memset(moved, 0, sizeof(moved));

	/*TRACE(state.bug << "Scheduling Guardians" << endl);
	schedGuardians(); checkTimer();*/

	TRACE(state.bug << "Scheduling Defenders" << endl);
	schedDefenders(); checkTimer();

	TRACE(state.bug << "Scheduling Fleers" << endl);
	schedFleers(); checkTimer();

	TRACE(state.bug << "Scheduling FoodFetchers" << endl);
	schedFoodFetchers(); checkTimer();

	if(state.myAnts.size() > minColonyForSpartans) {
		TRACE(state.bug << "Scheduling Spartans" << endl);
		schedSpartans(); checkTimer();
	}

	if(!explored) {
		TRACE(state.bug << "Scheduling Explorers" << endl);
		schedExplorers(); checkTimer();
	} else {
		TRACE(state.bug << "Scheduling Patrollers" << endl);
		schedPatrollers(); checkTimer();
	}


	for(int ant=0; ant<(int)state.myAnts.size(); ant++)
	{
		Location antLoc = state.myAnts[ant];

		// if not yet moved
		if(isFreeAnt(antLoc)) {
			if(hillDist(ant) < 10) {
				firstFree(ant);
				//avoider(ant);
			} else {
				randWalk(ant);
			}
			checkTimer();
		}
	}

    state.bug << "time taken: " << state.timer.getTime() << "ms" << endl << endl;
};

//finishes the turn
void Bot::endTurn()
{
    if(state.turn > 0)
        state.reset();
    state.turn++;

    cout << "go" << endl;
};


int Bot::bfs(Location start, Location target)
{
	std::queue<Location> q;

	memset(mark, 0, sizeof(int)*state.rows*state.cols);
	memset(pred, -1, sizeof(int)*state.rows*state.cols);

	q.push(start);
	mark[m(start.row, start.col)]++;
	pred[m(start.row, start.col)] = -1;


	while(!q.empty()) {
		Location cur = q.front();
		q.pop();

		if(cur == target) break;

		for(int d = 0; d < TDIRECTIONS; ++d) {
			Location n = state.getLocation(cur, d);
			if(!mark[m(n.row, n.col)]) {
				pred[m(n.row, n.col)] = d;
				mark[m(n.row, n.col)] = mark[m(cur.row, cur.col)]+1;

				if(isSeen(n) && isClear(n)) {
					q.push(n);
				} else if(n == target) {
					return 1;
				}
			}
		}
	}


/*
#ifdef DEBUG
	for(int i = 0; i < state.rows; ++i) {
		for(int j = 0; j < state.cols; ++j) {
			int p = (i * state.cols) + j;
			if(mark[p] == 0) {
				state.bug << "?";
			} else {
				state.bug << "*";
//				state.bug << mark[p];
			}
		}
		state.bug << endl;
	}
#endif // DEBUG
*/

	return 0;
}

Location Bot::explorerBfs(Location ant)
{
	std::queue<Location> q;

	memset(mark, 0, sizeof(int)*state.rows*state.cols);
	memset(pred, -1, sizeof(int)*state.rows*state.cols);

	q.push(ant);
	mark[m(ant.row, ant.col)]++;
	pred[m(ant.row, ant.col)] = -1;


	while(!q.empty()) {
		Location cur = q.front();
		q.pop();

		for(int d = 0; d < TDIRECTIONS; ++d) {
			Location n = state.getLocation(cur, d);

			if(!mark[m(n.row, n.col)] && isClear(n)) {
				pred[m(n.row, n.col)] = d;
				mark[m(n.row, n.col)] = 1;

				q.push(n);
			} else if(!isSeen(n)) {
				pred[m(n.row, n.col)] = d;
				return n;
			}
		}
	}

	return Location(-1, -1);
}


int Bot::nearbyBfs(Location start, int what, int dist)
{
	int cnt = 0;
	
	std::queue<std::pair<Location, int> > q;

	memset(mark, 0, sizeof(int)*state.rows*state.cols);
	memset(pred, -1, sizeof(int)*state.rows*state.cols);

	q.push(std::make_pair(start, 0));
	mark[m(start.row, start.col)]++;
	pred[m(start.row, start.col)] = -1;


	while(!q.empty()) {
		Location cur = q.front().first;
		int cdist = q.front().second;
		q.pop();

		if(cdist > dist) continue;

		for(int d = 0; d < TDIRECTIONS; ++d) {
			Location n = state.getLocation(cur, d);

			if(state.grid[cur.row][cur.col].ant == 0 && what == 0) {
				cnt++;
			} else if(state.grid[cur.row][cur.col].ant > 0 && what) {
				cnt++;
			}

			if(!mark[m(n.row, n.col)] && isVisible(n)) {
				pred[m(n.row, n.col)] = d;
				mark[m(n.row, n.col)] = mark[m(cur.row, cur.col)]+1;
				q.push(make_pair(n, cdist+1));
			}
		}
	}

	return cnt;
}


Location Bot::foodBfs(Location food)
{
	std::queue<Location> q;

	memset(mark, 0, sizeof(int)*state.rows*state.cols);
	memset(pred, -1, sizeof(int)*state.rows*state.cols);

	q.push(food);
	mark[m(food.row, food.col)]++;
	pred[m(food.row, food.col)] = -1;


	while(!q.empty()) {
		Location cur = q.front();
		q.pop();

		/*if(isFreeAnt(cur)) {
		  state.bug << "Found a fetcher!" << endl;
		  foodDirection = invdir(pred[m(cur.row, cur.col)]);
		  return cur;
		}*/

		for(int d = 0; d < TDIRECTIONS; ++d) {
			Location n = state.getLocation(cur, d);
			/*state.bug << n
			          << " m" << mark[m(n.row, n.col)]
					  << " c" << isClear(n)
					  << " v" << isVisible(n)
					  << " s" << isSeen(n) << endl;*/
			if(!mark[m(n.row, n.col)] && isSeen(n) && isClear(n)) {
				pred[m(n.row, n.col)] = d;
				mark[m(n.row, n.col)] = mark[m(cur.row, cur.col)]+1;
				q.push(n);
			} else if(isFreeAnt(n)) {
				foodDirection = invdir(d);
				return n;
			}
		}
	}

/*
#ifdef DEBUG
	for(int i = 0; i < state.rows; ++i) {
		for(int j = 0; j < state.cols; ++j) {
			int p = (i * state.cols) + j;
			if(food == Location(i, j)) {
				state.bug << "@";
			} else {
				if(mark[p] == 0) {
					state.bug << "?";
				} else {
					state.bug << "*";
					//				state.bug << mark[p]%10;
				}
			}
		}
		state.bug << endl;
	}
#endif // DEBUG
*/

	return Location(-1, -1);
}


Location Bot::patrollerBfs(Location ant)
{
	std::queue<Location> q;

	memset(mark, 0, sizeof(int)*state.rows*state.cols);
	memset(pred, -1, sizeof(int)*state.rows*state.cols);

	q.push(ant);
	mark[m(ant.row, ant.col)]++;
	pred[m(ant.row, ant.col)] = -1;


	while(!q.empty()) {
		Location cur = q.front();
		q.pop();

		for(int d = 0; d < TDIRECTIONS; ++d) {
			Location n = state.getLocation(cur, d);

			if(!mark[m(n.row, n.col)] && isClear(n)) {
				pred[m(n.row, n.col)] = d;
				mark[m(n.row, n.col)] = mark[m(cur.row, cur.col)]+1;
				q.push(n);
			} else if(!isVisible(n)) {
				pred[m(n.row, n.col)] = d;
				return n;
			}
		}
	}

	return Location(-1, -1);
}


std::vector<Location> Bot::spartanBfs(Location food)
{
	std::queue<Location> q;
	std::vector<Location> ans;

	memset(mark, 0, sizeof(int)*state.rows*state.cols);
	memset(pred, -1, sizeof(int)*state.rows*state.cols);

	q.push(food);
	mark[m(food.row, food.col)]++;
	pred[m(food.row, food.col)] = -1;


	while(!q.empty() && ans.size() < numSpartans) {
		Location cur = q.front();
		q.pop();

//		state.bug << "ant: " << state.grid[cur.row][cur.col].ant<< endl;
/*		if(isFreeAnt(cur)) {
			int hillDirection = invdir(pred[m(cur.row, cur.col)]);
//			move(cur, hillDirection);
			dir[cur.row][cur.col] = hillDirection;
			ans.push_back(cur);
		}*/

		for(int d = 0; d < TDIRECTIONS; ++d) {
			Location n = state.getLocation(cur, d);

			if(!mark[m(n.row, n.col)]) {
				mark[m(n.row, n.col)] = mark[m(n.row, n.col)]+1;

				if(isSeen(n) && isClear(n)) {
					pred[m(n.row, n.col)] = d;
					q.push(n);
				} else if(isFreeAnt(n)) {
					dir[n.row][n.col] = invdir(d);
					ans.push_back(n);
				}
			}
		}
	}
/*
#ifdef DEBUG
	for(int i = 0; i < state.rows; ++i) {
		for(int j = 0; j < state.cols; ++j) {
			int p = (i * state.cols) + j;
			if(mark[p] == 0) {
				state.bug << "?";
			} else {
//				state.bug << "*";
				state.bug << mark[p]%10;
			}
		}
		state.bug << endl;
	}
#endif // DEBUG
*/

#ifdef DEBUG
	state.bug << "Spartans:" << endl;
	for(int i = 0; i < ans.size(); ++i) {
		state.bug << ans[i] << endl;
	}
	state.bug << endl;
#endif // DEBUG

	return ans;
}


void Bot::move(Location location, int dir)
{
	Location n = state.getLocation(location, dir);

#ifdef DEBUG
	if(!isFreeAnt(location)) state.bug << "Not a free ant!" << endl;
	assert(isFreeAnt(location)); // Há formiga sem ordens na loc. atual
	if(!isClear(n)) state.bug << "Destiny not free!" << endl;
	assert(isClear(n)); // O destino está vazio
#endif // DEBUG

	state.makeMove(location, dir);
	moved[n.row][n.col] = 1;
}

void Bot::stay(Location location)
{
#ifdef DEBUG
	assert(isFreeAnt(location)); // Há formiga sem ordens na loc. atual
#endif // DEBUG

	moved[location.row][location.col] = 1;
}


inline bool Bot::isFood(Location loc)
{
	return state.grid[loc.row][loc.col].isFood;
}

inline bool Bot::isWater(Location loc)
{
	return state.grid[loc.row][loc.col].isWater;
}

inline bool Bot::isClear(Location loc)
{
	return !state.grid[loc.row][loc.col].isWater && state.grid[loc.row][loc.col].ant==-1;
}

inline bool Bot::isVisible(Location loc)
{
	return state.grid[loc.row][loc.col].isVisible;
}


inline bool Bot::isFreeAnt(Location loc)
{
	return state.grid[loc.row][loc.col].ant == 0 && !moved[loc.row][loc.col];
}


inline bool Bot::isSeen(Location loc)
{
	return state.grid[loc.row][loc.col].seen;
}


inline Location Bot::north(int ant)
{
	return state.getLocation(state.myAnts[ant], 0);
}


inline Location Bot::east(int ant)
{
	return state.getLocation(state.myAnts[ant], 1);
}


inline Location Bot::south(int ant)
{
	return state.getLocation(state.myAnts[ant], 2);
}


inline Location Bot::west(int ant)
{
	return state.getLocation(state.myAnts[ant], 3);
}


int Bot::hillDist(int ant)
{
	int dist = 10000000;
	Location antLoc = state.myAnts[ant];

	for(int i = 0; i < state.myHills.size(); ++i) {
		Location delta = antLoc-state.myHills[i];

		dist = min(dist, abs(delta.row)+abs(delta.col));
	}

	return dist;
}


Location Bot::nearestEnemy(Location loc)
{
	Location nearest(-1, -1);

	int mindist=1000000;
	for(std::vector<Location>::iterator it = state.enemyAnts.begin(), end = state.enemyAnts.end();
	    it != end;
		++it) {
		int d = norm1(*it-loc);
		if(d < mindist) {
			mindist = d;
			nearest = *it;
		}
	}

	return nearest;
}


void Bot::schedDefenders()
{
	for(std::vector<Location>::iterator it = state.myHills.begin(), end = state.myHills.end();
			it != end; ++it) {
		Location enemy = nearestEnemy(*it);
		if(state.distance(enemy, *it) < 10) {
			TRACE(state.bug << "Defenders needed" << endl);
			std::vector<Location> s = spartanBfs(enemy);

			for(int i = 0; i < s.size(); ++i) {
				Location antLoc = s[i];
				if(isClear(state.getLocation(antLoc, dir[antLoc.row][antLoc.col]))) {
					move(s[i], dir[antLoc.row][antLoc.col]);
				}
			}
		}
	}
}


void Bot::schedExplorers()
{
	for(locvec::iterator it = state.myAnts.begin(), end = state.myAnts.end();
			it != end;
			++it) {

		if(isFreeAnt(*it)) {

			Location c = explorerBfs(*it);

			if(c != Location(-1, -1)) {
				// se achou um tile inexplorado

				TRACE(state.bug << "Unexplored tile at " << c << endl);

				if(bfs(c, *it)) {
				// Se há caminho

					TRACE(state.bug << "Found a route, sending explorer" << endl);

					int dir = invdir(pred[m(it->row, it->col)]);

					/*int pd = -1;
					  while(c != *it) {
					  pd = invdir(pred[m(c.row, c.col)]);
					  c = state.getLocation(c, pd);
					  state.bug << c << " " << pd << endl;
					  }

					  state.bug << "Saiu do while" << endl;

					  int dir = invdir(pd);
					 */
				#ifdef DEBUG
					Location n = state.getLocation(*it, dir);
					assert(isClear(n));
				#endif // DEBUG
					move(*it, dir);
				}
			}
		}

		checkTimer();
	}

	bool allSeen = true;

	for(int i = 0; i < state.rows && allSeen; ++i) {
		for(int j = 0; j < state.cols && allSeen; ++j) {
			if(!isSeen(Location(i, j))) {
				allSeen = false;
			}
		}
	}

	if(allSeen) explored = true;
}


void Bot::schedFleers()
{
	for(locvec::iterator it = state.myAnts.begin(), end = state.myAnts.end();
	    it != end;
		++it) {

		int enemy = nearbyBfs(*it, 1, 5);
		int friendly = nearbyBfs(*it, 0, 5);

		if(friendly - enemy < 3) {
			stay(*it);
		}

		checkTimer();
	}
}


void Bot::schedFoodFetchers()
{
	for(int i = 0; i < state.food.size(); ++i) {
		Location foodLoc = state.food[i];

		Location antLoc = foodBfs(foodLoc);

		if(antLoc != Location(-1, -1)) {
			move(antLoc, foodDirection);
		}
	}
}


void Bot::schedGuardians()
{
	int cnt = 1;
	for(locvec::iterator it = state.myHills.begin(), end = state.myHills.end();
	    it != end;
		++it) {

		if(state.myAnts.size() < minColonyForGuardians*cnt) break;
		cnt++;

		Location g[4];
		g[0] = state.getLocation(state.getLocation(*it, 0), 1);
		g[1] = state.getLocation(state.getLocation(*it, 0), 3);
		g[2] = state.getLocation(state.getLocation(*it, 2), 1);
		g[3] = state.getLocation(state.getLocation(*it, 2), 3);

		for(int i = 0; i < 4; ++i) {
			if(isClear(g[i])) {
			// Se não há formigas na posição de guarda

				Location antLoc = foodBfs(g[i]);

				if(antLoc != Location(-1, -1)) {
					move(antLoc, foodDirection);
				}
			} else {
				// if already on guard, keep it
				moved[g[i].row][g[i].col] = 1;
			}
		}
	}
}


void Bot::schedPatrollers()
{
	for(locvec::iterator it = state.myAnts.begin(), end = state.myAnts.end();
			it != end;
			++it) {

		if(isFreeAnt(*it)) {

			Location c = explorerBfs(*it);

			if(c != Location(-1, -1)) {
				// se achou um tile inexplorado
				if(bfs(c, *it)) {
					// Se há caminho

					int dir = invdir(pred[m(it->row, it->col)]);

					/*int pd = -1;
					  while(c != *it) {
					  pd = invdir(pred[m(c.row, c.col)]);
					  c = state.getLocation(c, pd);
					  state.bug << c << " " << pd << endl;
					  }

					  state.bug << "Saiu do while" << endl;

					  int dir = invdir(pd);
					 */
				#ifdef DEBUG
					Location n = state.getLocation(*it, dir);
					assert(isClear(n));
				#endif // DEBUG
					move(*it, dir);
				}
			}
		}

		checkTimer();
	}
}


void Bot::schedSpartans()
{
	for(int i = 0; i < state.enemyHills.size(); ++i) {
		std::vector<Location> s = spartanBfs(state.enemyHills[i]);

		if(s.size() > minSpartans) {
			for(int i = 0; i < s.size(); ++i) {
				Location antLoc = s[i];
				if(isClear(state.getLocation(antLoc, dir[antLoc.row][antLoc.col]))) {
					move(s[i], dir[antLoc.row][antLoc.col]);
				}
			}
		}
	}
}


void Bot::avoider(int ant)
{
	int occ = 0;
	Location antLoc = state.myAnts[ant];
	int occdir;
	int cleardir;

	for(int d = 0; d < TDIRECTIONS; ++d) {
		Location loc = state.getLocation(antLoc, d);

		if(!isClear(loc)) {
			occ++;
			occdir = d;
		} else {
			cleardir = d;
		}
	}

	switch(occ) {
		case 0:
			break;

		case 1:
			move(antLoc, invdir(occdir));
			break;

		case 2:
		case 3:
			move(antLoc, cleardir);
			break;

		case 4:
			break;
	}
}


void Bot::clockWise(int ant)
{
}


void Bot::explore(int ant)
{
	Location antLoc = state.myAnts[ant];

	for(int i = 0; i < 10; ++i) {
		int d = rand()%4;

		Location loc = state.getLocation(antLoc, d);

		if(isClear(loc)) {
			move(antLoc, d);
			break;
		}
	}
}


void Bot::firstFree(int ant)
{
	Location antLoc = state.myAnts[ant];

	for(int d = 0; d < TDIRECTIONS; ++d) {
		int di = (ant+d)%4;
		Location loc = state.getLocation(antLoc, di);

		if(isClear(loc)) {
			move(antLoc, di);
			break;
		}
	}
}


void Bot::randWalk(int ant)
{
	Location antLoc = state.myAnts[ant];

	for(int i = 0; i < 10; ++i) {
		int d = rand()%4;

		Location loc = state.getLocation(antLoc, d);

		if(isClear(loc)) {
			move(antLoc, d);
			break;
		}
	}
}


