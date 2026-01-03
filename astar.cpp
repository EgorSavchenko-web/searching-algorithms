#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <tuple>
#include <algorithm>
#include <sstream>
using namespace std;

const int SIZE = 13; // grid size

// map symbols
char ORC = 'O';
char URUK = 'U';
char NAZGUL = 'N';
char WATCH = 'W';
char PERCEP = 'P';
char RING = 'R';
char MITHRIL = 'C';
char GOLLUM = 'G';
char MOUNT = 'M';

struct Percept { int x; int y; char t; }; // simple percept record

int variant_number = 1;
int perception_range = 1;
int goal_x = -1, goal_y = -1;
int mount_x = -1, mount_y = -1;
bool knows_mount = false;

vector<string> world_map;
vector<vector<bool>> dangerous;
vector<vector<bool>> seen_cells;

int current_x = 0, current_y = 0;
bool ring_active = false;
bool has_mithril = false;
int move_count = 0;

// check coordinates inside grid
bool inside(int x, int y) { return x >= 0 && x < SIZE && y >= 0 && y < SIZE; }

// evaluate if a cell is unsafe given ring/mithril state
bool is_dangerous(int x, int y, bool ring, bool mithril) {
    if (!inside(x, y)) return true;
    char cell = world_map[x][y];
    if (cell == ORC || cell == URUK || cell == NAZGUL || cell == WATCH) return true;
    if (dangerous[x][y]) return true;

    // check ranges of all enemies on the map
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            char enemy = world_map[i][j];
            if (enemy == ORC) {
                int range = (mithril || ring) ? 0 : 1;
                int dist = abs(i - x) + abs(j - y);
                if (dist <= range) return true;
            } else if (enemy == URUK) {
                int range = 2;
                if (mithril || ring) range = range - 1;
                if (range < 0) range = 0;
                int dist = abs(i - x) + abs(j - y);
                if (dist <= range) return true;
            } else if (enemy == NAZGUL) {
                int range = ring ? 2 : 1;
                int dist = max(abs(i - x), abs(j - y));
                if (dist <= range) return true;
            } else if (enemy == WATCH) {
                int range = ring ? 3 : 2;
                int dist = max(abs(i - x), abs(j - y));
                if (dist <= range) return true;
            }
        }
    }
    return false;
}

// mark cells visible from current position
void mark_seen() {
    for (int dx = -perception_range; dx <= perception_range; dx++) {
        for (int dy = -perception_range; dy <= perception_range; dy++) {
            if (max(abs(dx), abs(dy)) <= perception_range) {
                int nx = current_x + dx;
                int ny = current_y + dy;
                if (inside(nx, ny)) seen_cells[nx][ny] = true;
            }
        }
    }
}

// apply percepts to local world model
void update_world(const vector<Percept>& percepts) {
    for (int i = 0; i < percepts.size(); i++) {
        Percept p = percepts[i];
        if (!inside(p.x, p.y)) continue;
        if (p.t == PERCEP) dangerous[p.x][p.y] = true;
        else if (p.t == MITHRIL) world_map[p.x][p.y] = MITHRIL;
        else if (p.t == GOLLUM) world_map[p.x][p.y] = GOLLUM;
        else if (p.t == MOUNT) { world_map[p.x][p.y] = MOUNT; mount_x = p.x; mount_y = p.y; knows_mount = true; }
        else if (p.t == ORC || p.t == URUK || p.t == NAZGUL || p.t == WATCH) {
            world_map[p.x][p.y] = p.t; dangerous[p.x][p.y] = true;
        }
    }
}

// read percept list from input
vector<Percept> read_percepts() {
    vector<Percept> result;
    int count; cin >> count;
    for (int i = 0; i < count; i++) {
        int x, y; string type; cin >> x >> y >> type;
        if (type.size() > 0) result.push_back({x, y, type[0]});
    }
    return result;
}

// try to parse Mount Doom coordinates from a line
void try_find_mount() {
    string line; getline(cin, line);
    if (!getline(cin, line)) return;
    stringstream ss(line); int x, y; if (ss >> x >> y) {
        mount_x = x; mount_y = y; knows_mount = true; if (inside(x, y)) world_map[x][y] = MOUNT;
    }
}

struct Step { char action; int x; int y; }; // plan step: 'M' move, 'R' ring on, 'O' ring off

// A* search over extended state (x,y,ring,mithril)
bool find_path(int start_x, int start_y, int target_x, int target_y,
               bool start_ring, bool start_mithril, vector<Step>& path) {
    const int INF = 1000000;
    int dist[SIZE][SIZE][2][2];               // g-values
    int prev_x[SIZE][SIZE][2][2];
    int prev_y[SIZE][SIZE][2][2];
    int prev_r[SIZE][SIZE][2][2];
    int prev_m[SIZE][SIZE][2][2];
    char action_taken[SIZE][SIZE][2][2];

    // initialize distances and predecessors
    for (int i = 0; i < SIZE; i++) for (int j = 0; j < SIZE; j++) for (int r = 0; r < 2; r++) for (int m = 0; m < 2; m++) {
                    dist[i][j][r][m] = INF; prev_x[i][j][r][m] = -1; prev_y[i][j][r][m] = -1; prev_r[i][j][r][m] = -1; prev_m[i][j][r][m] = -1; action_taken[i][j][r][m] = ' ';
                }

    int start_r = start_ring ? 1 : 0; int start_m = start_mithril ? 1 : 0;
    dist[start_x][start_y][start_r][start_m] = 0;

    // open list implemented as vector with manual best-f selection
    vector<tuple<int,int,int,int>> nodes;
    nodes.push_back(make_tuple(start_x, start_y, start_r, start_m));

    int dx[4] = {-1, 0, 1, 0};
    int dy[4] = {0, 1, 0, -1};

    while (!nodes.empty()) {
        // pick node with smallest f = g + h (manhattan heuristic)
        int best_index = -1; int best_f = INF;
        for (int i = 0; i < nodes.size(); i++) {
            int x = get<0>(nodes[i]); int y = get<1>(nodes[i]); int r = get<2>(nodes[i]); int m = get<3>(nodes[i]);
            int g = dist[x][y][r][m];
            int h = abs(x - target_x) + abs(y - target_y);
            int f = g + h;
            if (f < best_f) { best_f = f; best_index = i; }
        }

        if (best_index == -1) break;

        int x = get<0>(nodes[best_index]); int y = get<1>(nodes[best_index]); int r = get<2>(nodes[best_index]); int m = get<3>(nodes[best_index]);
        nodes.erase(nodes.begin() + best_index);

        // goal reached -> reconstruct path
        if (x == target_x && y == target_y) {
            path.clear();
            int cur_x = x, cur_y = y, cur_r = r, cur_m = m;
            while (cur_x != start_x || cur_y != start_y || cur_r != start_r || cur_m != start_m) {
                char act = action_taken[cur_x][cur_y][cur_r][cur_m];
                if (act == 'M') path.push_back({'M', cur_x, cur_y});
                else if (act == 'R') path.push_back({'R', -1, -1});
                else if (act == 'O') path.push_back({'O', -1, -1});
                int px = prev_x[cur_x][cur_y][cur_r][cur_m];
                int py = prev_y[cur_x][cur_y][cur_r][cur_m];
                int pr = prev_r[cur_x][cur_y][cur_r][cur_m];
                int pm = prev_m[cur_x][cur_y][cur_r][cur_m];
                cur_x = px; cur_y = py; cur_r = pr; cur_m = pm;
            }
            reverse(path.begin(), path.end());
            return true;
        }

        bool ring = (r == 1);
        bool mithril = (m == 1);

        // zero-cost actions: toggle ring on/off (modeled as same-cost transitions)
        if (!ring) {
            int new_r = 1, new_m = m;
            if (dist[x][y][new_r][new_m] > dist[x][y][r][m]) {
                dist[x][y][new_r][new_m] = dist[x][y][r][m];
                prev_x[x][y][new_r][new_m] = x; prev_y[x][y][new_r][new_m] = y; prev_r[x][y][new_r][new_m] = r; prev_m[x][y][new_r][new_m] = m;
                action_taken[x][y][new_r][new_m] = 'R'; nodes.push_back(make_tuple(x, y, new_r, new_m));
            }
        }
        if (ring) {
            int new_r = 0, new_m = m;
            if (dist[x][y][new_r][new_m] > dist[x][y][r][m]) {
                dist[x][y][new_r][new_m] = dist[x][y][r][m];
                prev_x[x][y][new_r][new_m] = x; prev_y[x][y][new_r][new_m] = y; prev_r[x][y][new_r][new_m] = r; prev_m[x][y][new_r][new_m] = m;
                action_taken[x][y][new_r][new_m] = 'O'; nodes.push_back(make_tuple(x, y, new_r, new_m));
            }
        }

        // explore 4-neighbors (cost +1)
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + dx[dir]; int ny = y + dy[dir];
            if (!inside(nx, ny)) continue;
            if (is_dangerous(nx, ny, ring, mithril)) continue; // skip unsafe

            int new_m = m; if (world_map[nx][ny] == MITHRIL) new_m = 1; int new_r = r;
            if (dist[nx][ny][new_r][new_m] > dist[x][y][r][m] + 1) {
                dist[nx][ny][new_r][new_m] = dist[x][y][r][m] + 1;
                prev_x[nx][ny][new_r][new_m] = x; prev_y[nx][ny][new_r][new_m] = y; prev_r[nx][ny][new_r][new_m] = r; prev_m[nx][ny][new_r][new_m] = m;
                action_taken[nx][ny][new_r][new_m] = 'M';
                nodes.push_back(make_tuple(nx, ny, new_r, new_m));
            }
        }
    }
    return false; // no path found
}

// perform move command and update world
bool make_move(int new_x, int new_y) {
    cout << "m " << new_x << " " << new_y << endl; cout.flush();
    current_x = new_x; current_y = new_y; move_count++;
    vector<Percept> percepts = read_percepts(); update_world(percepts); mark_seen();
    if (world_map[current_x][current_y] == MITHRIL) has_mithril = true;
    if (current_x == goal_x && current_y == goal_y && !knows_mount) { try_find_mount(); world_map[current_x][current_y] = GOLLUM; }
    if (world_map[current_x][current_y] == ORC || world_map[current_x][current_y] == URUK || world_map[current_x][current_y] == NAZGUL || world_map[current_x][current_y] == WATCH) return false;
    if (dangerous[current_x][current_y]) return false;
    return true;
}

// toggle ring and read new perceptions
bool toggle_ring(bool turn_on) {
    if (turn_on == ring_active) return true;
    if (turn_on) cout << "r" << endl; else cout << "rr" << endl;
    cout.flush(); ring_active = turn_on;
    vector<Percept> percepts = read_percepts(); update_world(percepts); mark_seen();
    if (world_map[current_x][current_y] == MITHRIL) has_mithril = true;
    if (current_x == goal_x && current_y == goal_y && !knows_mount) try_find_mount();
    if (world_map[current_x][current_y] == ORC || world_map[current_x][current_y] == URUK || world_map[current_x][current_y] == NAZGUL || world_map[current_x][current_y] == WATCH) return false;
    if (dangerous[current_x][current_y]) return false;
    if (is_dangerous(current_x, current_y, ring_active, has_mithril)) return false;
    return true;
}

int main() {
    // initialize maps
    world_map.resize(SIZE, string(SIZE, '.'));
    dangerous.resize(SIZE, vector<bool>(SIZE, false));
    seen_cells.resize(SIZE, vector<bool>(SIZE, false));

    cin >> variant_number;
    perception_range = (variant_number == 1) ? 1 : 2;
    cin >> goal_x >> goal_y; if (inside(goal_x, goal_y)) world_map[goal_x][goal_y] = GOLLUM;

    // read initial percepts and mark visible cells
    vector<Percept> initial_percepts = read_percepts(); update_world(initial_percepts); mark_seen(); seen_cells[0][0] = true;
    if (current_x == goal_x && current_y == goal_y && !knows_mount) try_find_mount();

    while (true) {
        int target_x, target_y;
        if (!knows_mount) { target_x = goal_x; target_y = goal_y; }
        else { target_x = mount_x; target_y = mount_y; }

        if (knows_mount && current_x == mount_x && current_y == mount_y) { cout << "e " << move_count << endl; cout.flush(); return 0; }

        vector<Step> plan;
        bool found_path = find_path(current_x, current_y, target_x, target_y, ring_active, has_mithril, plan);

        if (!found_path) {
            // greedy exploration: pick neighbor that reveals most unseen cells
            vector<pair<int,int>> possible_moves;
            int dx[4] = {-1,0,1,0}; int dy[4] = {0,1,0,-1};
            for (int i = 0; i < 4; i++) {
                int nx = current_x + dx[i]; int ny = current_y + dy[i];
                if (!inside(nx, ny)) continue;
                if (world_map[nx][ny] == ORC || world_map[nx][ny] == URUK || world_map[nx][ny] == NAZGUL || world_map[nx][ny] == WATCH) continue;
                if (dangerous[nx][ny]) continue;
                if (is_dangerous(nx, ny, ring_active, has_mithril)) continue;
                possible_moves.push_back(make_pair(nx, ny));
            }
            if (possible_moves.empty()) { cout << "e -1" << endl; cout.flush(); return 0; }

            int best_score = -1; pair<int,int> best_move;
            for (int i = 0; i < possible_moves.size(); i++) {
                int nx = possible_moves[i].first; int ny = possible_moves[i].second; int new_info = 0;
                for (int dx = -perception_range; dx <= perception_range; dx++) for (int dy = -perception_range; dy <= perception_range; dy++) if (max(abs(dx), abs(dy)) <= perception_range) {
                            int px = nx + dx; int py = ny + dy; if (inside(px, py) && !seen_cells[px][py]) new_info++;
                        }
                if (new_info > best_score) { best_score = new_info; best_move = possible_moves[i]; }
            }
            if (best_score == 0) { cout << "e -1" << endl; cout.flush(); return 0; }
            bool success = make_move(best_move.first, best_move.second);
            if (!success) { cout << "e -1" << endl; cout.flush(); return 0; }
            continue;
        }

        // execute planned steps
        for (int i = 0; i < plan.size(); i++) {
            Step step = plan[i];
            if (step.action == 'R' || step.action == 'O') {
                bool want_ring = (step.action == 'R');
                bool success = toggle_ring(want_ring); if (!success) { cout << "e -1" << endl; cout.flush(); return 0; }
            } else if (step.action == 'M') {
                if (abs(step.x - current_x) + abs(step.y - current_y) != 1) break; // invalid plan
                if (is_dangerous(step.x, step.y, ring_active, has_mithril)) break; // safety check
                bool success = make_move(step.x, step.y); if (!success) { cout << "e -1" << endl; cout.flush(); return 0; }
            }
            if (i + 1 < plan.size()) {
                Step next_step = plan[i + 1]; if (next_step.action == 'M') if (is_dangerous(next_step.x, next_step.y, ring_active, has_mithril)) break;
            }
            if (!knows_mount && current_x == goal_x && current_y == goal_y) break;
        }
    }
    return 0;
}