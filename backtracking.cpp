#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

using namespace std;

const int SIZE = 13;               // grid size
const int BIG_NUMBER = 1000000000; // large constant for initialization

// world representation and agent state
string map[SIZE][SIZE];
bool danger[SIZE][SIZE];
int gollum_x, gollum_y;            // Gollum position
int mount_doom_x = -1, mount_doom_y = -1; // Mount Doom position (if found)
bool found_mount_doom = false;
int current_x = 0, current_y = 0;  // agent current position
bool ring_active = false;          // ring state
bool has_mithril = false;          // collected mithril
int total_moves = 0;

// memoization for best known distances: [x][y][ring][mithril]
int best_distance[SIZE][SIZE][2][2];
int shortest_path = BIG_NUMBER;    // best path length found so far

// check coordinates are inside the map
bool is_inside(int x, int y) {
    return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

// read perceived cells from input
vector<vector<string>> get_surroundings() {
    vector<vector<string>> result;
    int count;
    cin >> count;
    for (int i = 0; i < count; i++) {
        int x, y;
        string type;
        cin >> x >> y >> type;
        vector<string> item;
        item.push_back(to_string(x));
        item.push_back(to_string(y));
        item.push_back(type);
        result.push_back(item);
    }
    return result;
}

// try to parse Mount Doom coordinates from a line
void check_for_mount_doom() {
    string line;
    getline(cin, line);
    if (getline(cin, line)) {
        stringstream ss(line);
        string word;
        int numbers_found = 0;
        int temp_x = -1, temp_y = -1;
        while (ss >> word) {
            try {
                int num = stoi(word);
                if (numbers_found == 0) {
                    temp_x = num;
                    numbers_found++;
                } else if (numbers_found == 1) {
                    temp_y = num;
                    numbers_found++;
                    break;
                }
            } catch (...) {
                // ignore non-numeric tokens
            }
        }
        if (temp_x != -1 && temp_y != -1) {
            mount_doom_x = temp_x;
            mount_doom_y = temp_y;
            found_mount_doom = true;
            if (is_inside(mount_doom_x, mount_doom_y)) {
                map[mount_doom_x][mount_doom_y] = "M";
            }
        }
    }
}

// update internal map and danger flags from perceptions
void update_knowledge(vector<vector<string>> perceptions) {
    for (auto item : perceptions) {
        int x = stoi(item[0]);
        int y = stoi(item[1]);
        string t = item[2];
        if (!is_inside(x, y)) continue;
        if (t == "P") {
            danger[x][y] = true;            // percept: nearby danger
        } else if (t == "C") {
            map[x][y] = "C";             // mithril
        } else if (t == "G") {
            map[x][y] = "G";             // Gollum
        } else if (t == "M") {
            map[x][y] = "M";             // Mount Doom
            mount_doom_x = x;
            mount_doom_y = y;
            found_mount_doom = true;
        } else if (t == "O" || t == "U" || t == "N" || t == "W") {
            map[x][y] = t;                // enemy type
            danger[x][y] = true;
        }
    }
}

// check if cell (x,y) is dangerous given ring/mithril state
bool is_dangerous(int x, int y, bool ring, bool mithril) {
    if (!is_inside(x, y)) return true;
    string cell_type = map[x][y];
    if (cell_type == "O" || cell_type == "U" || cell_type == "N" || cell_type == "W") {
        return true; // occupied by enemy
    }
    if (danger[x][y]) {
        return true; // marked dangerous by percept
    }
    // evaluate enemy ranges on the map
    for (int ex = 0; ex < SIZE; ex++) {
        for (int ey = 0; ey < SIZE; ey++) {
            string enemy_type = map[ex][ey];
            if (enemy_type != "O" && enemy_type != "U" && enemy_type != "N" && enemy_type != "W") {
                continue;
            }
            if (enemy_type == "O") {
                int range = (ring || mithril) ? 0 : 1;
                if (abs(ex - x) + abs(ey - y) <= range) return true;
            } else if (enemy_type == "U") {
                int range = (ring || mithril) ? 1 : 2;
                if (abs(ex - x) + abs(ey - y) <= range) return true;
            } else if (enemy_type == "N") {
                int range = ring ? 2 : 1;
                if (max(abs(ex - x), abs(ey - y)) <= range) return true;
            } else if (enemy_type == "W") {
                int range = ring ? 3 : 2;
                if (max(abs(ex - x), abs(ey - y)) <= range) return true;
            }
        }
    }
    return false;
}

// send move command, read perceptions and update state
bool move_to(int new_x, int new_y) {
    cout << "m " << new_x << " " << new_y << endl;
    cout.flush();
    current_x = new_x;
    current_y = new_y;
    total_moves++;
    vector<vector<string>> perceptions = get_surroundings();
    update_knowledge(perceptions);
    if (map[current_x][current_y] == "C") {
        has_mithril = true; // collect mithril
    }
    if (current_x == gollum_x && current_y == gollum_y && !found_mount_doom) {
        check_for_mount_doom(); // attempt to read Mount Doom coords
    }
    // fail if stepped into known enemy or dangerous cell
    if (map[current_x][current_y] == "O" || map[current_x][current_y] == "U" ||
        map[current_x][current_y] == "N" || map[current_x][current_y] == "W" ||
        danger[current_x][current_y]) {
        return false;
    }
    return true;
}

// toggle ring on/off, read new perceptions
bool toggle_ring(bool turn_on) {
    if (turn_on == ring_active) return true;
    if (turn_on) {
        cout << "r" << endl;   // turn ring on
    } else {
        cout << "rr" << endl;  // turn ring off
    }
    cout.flush();
    ring_active = turn_on;
    vector<vector<string>> perceptions = get_surroundings();
    update_knowledge(perceptions);
    if (current_x == gollum_x && current_y == gollum_y && !found_mount_doom) {
        check_for_mount_doom();
    }
    // check safety after toggling
    if (map[current_x][current_y] == "O" || map[current_x][current_y] == "U" ||
        map[current_x][current_y] == "N" || map[current_x][current_y] == "W" ||
        danger[current_x][current_y]) {
        return false;
    }
    if (is_dangerous(current_x, current_y, ring_active, has_mithril)) {
        return false;
    }
    return true;
}

// recursive backtracking search over (x,y,ring,mithril)
void search(int x, int y, bool ring, bool mithril, int path_length) {
    if (path_length >= shortest_path) return; // branch-and-bound
    int ring_index = ring ? 1 : 0;
    int mithril_index = mithril ? 1 : 0;
    if (path_length >= best_distance[x][y][ring_index][mithril_index]) return; // prune
    best_distance[x][y][ring_index][mithril_index] = path_length;

    // goal test: reached Mount Doom
    if (found_mount_doom && x == mount_doom_x && y == mount_doom_y) {
        if (path_length < shortest_path) shortest_path = path_length;
        return;
    }

    // try toggling ring off if currently on
    if (ring) {
        bool success = toggle_ring(false);
        if (success) {
            search(x, y, false, mithril, path_length);
            toggle_ring(true); // revert ring state
        }
    }
    // try toggling ring on if currently off
    if (!ring) {
        bool success = toggle_ring(true);
        if (success) {
            search(x, y, true, mithril, path_length);
            toggle_ring(false);
        }
    }

    // try moving to neighbors
    int moves[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    vector<vector<int>> safe_cells;
    vector<vector<int>> unknown_cells;
    for (int i = 0; i < 4; i++) {
        int nx = x + moves[i][0];
        int ny = y + moves[i][1];
        if (!is_inside(nx, ny)) continue;
        string cell_type = map[nx][ny];
        if (cell_type == "O" || cell_type == "U" || cell_type == "N" || cell_type == "W" || danger[nx][ny]) {
            continue; // skip known enemies/danger
        }
        if (is_dangerous(nx, ny, ring, mithril)) {
            continue; // skip cells that are dangerous under current state
        }
        if (cell_type == ".") unknown_cells.push_back({nx, ny});
        else safe_cells.push_back({nx, ny});
    }

    // prefer safe known cells, then unknown cells
    vector<vector<int>> cells_to_try;
    cells_to_try.insert(cells_to_try.end(), safe_cells.begin(), safe_cells.end());
    cells_to_try.insert(cells_to_try.end(), unknown_cells.begin(), unknown_cells.end());

    for (auto cell : cells_to_try) {
        int nx = cell[0];
        int ny = cell[1];
        bool new_mithril = mithril || (map[nx][ny] == "C");
        int new_ring_index = ring ? 1 : 0;
        int new_mithril_index = new_mithril ? 1 : 0;
        if (path_length + 1 >= best_distance[nx][ny][new_ring_index][new_mithril_index]) continue;

        bool success = move_to(nx, ny); // perform move (may read perceptions)
        if (success) {
            search(nx, ny, ring, new_mithril, path_length + 1);
            move_to(x, y); // backtrack: return to previous cell
        }
    }
}

int main() {
    // initialize map and best distances
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            map[i][j] = ".";
            danger[i][j] = false;
            for (int r = 0; r < 2; r++) {
                for (int m = 0; m < 2; m++) {
                    best_distance[i][j][r][m] = BIG_NUMBER;
                }
            }
        }
    }

    int variant;
    cin >> variant;
    cin >> gollum_x >> gollum_y; // read Gollum position
    if (is_inside(gollum_x, gollum_y)) map[gollum_x][gollum_y] = "G";

    vector<vector<string>> initial_perceptions = get_surroundings();
    update_knowledge(initial_perceptions);
    if (current_x == gollum_x && current_y == gollum_y && !found_mount_doom) {
        check_for_mount_doom();
    }

    if (danger[0][0]) { // start is unsafe
        cout << "e -1" << endl;
        return 0;
    }

    search(0, 0, false, false, 0); // start DFS

    if (shortest_path < BIG_NUMBER) cout << "e " << shortest_path << endl;
    else cout << "e -1" << endl;
    cout.flush();
    return 0;
}