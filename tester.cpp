#include <bits/stdc++.h>
#include <windows.h>
#include <chrono>
using namespace std;

const int N = 13;
const int NUM_TESTS = 1000;

struct Percept {
    int x, y;
    char t;
};

struct MapData {
    pair<int, int> gollum;
    pair<int, int> mount;
    pair<int, int> mithril;
    vector<tuple<int, int, char>> enemies;
};

bool is_lethal(int x, int y, bool ring, bool mith, const vector<tuple<int, int, char>>& enemies) {
    for (auto& e : enemies) {
        int ex, ey; char et;
        tie(ex, ey, et) = e;
        if (ex == x && ey == y) return true;
        int dist_m = abs(ex - x) + abs(ey - y);
        int dist_c = max(abs(ex - x), abs(ey - y));
        if (et == 'O') {
            int r = (ring || mith) ? 0 : 1;
            if (dist_m <= r) return true;
        } else if (et == 'U') {
            int r = (ring || mith) ? 1 : 2;
            if (dist_m <= r) return true;
        } else if (et == 'N') {
            int r = ring ? 2 : 1;
            if (dist_c <= r) return true;
        } else if (et == 'W') {
            int r = ring ? 3 : 2;
            if (dist_c <= r) return true;
        }
    }
    return false;
}

MapData generate_map() {
    set<pair<int, int>> positions;
    positions.insert({0, 0});

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(0, N - 1);

    auto rand_pos = [&](bool exclude_start = true) -> pair<int, int> {
        while (true) {
            int x = dist(gen);
            int y = dist(gen);
            if (exclude_start && x == 0 && y == 0) continue;
            if (positions.find({x, y}) == positions.end()) {
                positions.insert({x, y});
                return {x, y};
            }
        }
    };

    auto g_pos = rand_pos(true);
    auto m_pos = rand_pos(true);
    vector<tuple<int, int, char>> enemies;

    auto w_pos = rand_pos(false);
    enemies.emplace_back(w_pos.first, w_pos.second, 'W');

    auto u_pos = rand_pos(false);
    enemies.emplace_back(u_pos.first, u_pos.second, 'U');

    uniform_real_distribution<double> prob(0.0, 1.0);
    if (prob(gen) < 0.5) {
        auto n_pos = rand_pos(false);
        enemies.emplace_back(n_pos.first, n_pos.second, 'N');
    }

    uniform_int_distribution<int> num_o(1, 2);
    int no = num_o(gen);
    for (int i = 0; i < no; ++i) {
        auto o_pos = rand_pos(false);
        enemies.emplace_back(o_pos.first, o_pos.second, 'O');
    }

    auto c_pos = rand_pos(false);

    vector<pair<int, int>> key_pos = {{0, 0}, g_pos, m_pos, c_pos};
    for (auto& kp : key_pos) {
        if (is_lethal(kp.first, kp.second, false, false, enemies)) {
            return generate_map();
        }
    }

    return {g_pos, m_pos, c_pos, enemies};
}

vector<Percept> get_percepts(int curx, int cury, int r_percept, bool ring, bool mith, const MapData& map, bool have_mount) {
    vector<Percept> percepts;
    for (int ddx = -r_percept; ddx <= r_percept; ++ddx) {
        for (int ddy = -r_percept; ddy <= r_percept; ++ddy) {
            if (max(abs(ddx), abs(ddy)) > r_percept) continue;
            int px = curx + ddx;
            int py = cury + ddy;
            if (px < 0 || px >= N || py < 0 || py >= N) continue;
            char tok = 0;
            if (make_pair(px, py) == map.gollum) tok = 'G';
            else if (make_pair(px, py) == map.mithril) tok = 'C';
            else if (have_mount && make_pair(px, py) == map.mount) tok = 'M';
            else {
                for (auto& e : map.enemies) {
                    int ex, ey; char et;
                    tie(ex, ey, et) = e;
                    if (ex == px && ey == py) {
                        tok = et;
                        break;
                    }
                }
            }
            if (tok) {
                percepts.push_back({px, py, tok});
            } else if (is_lethal(px, py, ring, mith, map.enemies)) {
                percepts.push_back({px, py, 'P'});
            }
        }
    }
    return percepts;
}

pair<int, double> run_algo(const string& algo_name, int variant, const MapData& map) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStd_IN_Rd = NULL;
    HANDLE hChildStd_IN_Wr = NULL;
    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;

    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
        return {-2, 0.0};
    }
    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        return {-2, 0.0};
    }

    if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0)) {
        return {-2, 0.0};
    }
    if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
        return {-2, 0.0};
    }

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    bool bSuccess = false;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    string cmd = algo_name + ".exe";

    bSuccess = CreateProcess(NULL,
                             (LPSTR)cmd.c_str(),
                             NULL,
                             NULL,
                             TRUE,
                             0,
                             NULL,
                             NULL,
                             &siStartInfo,
                             &piProcInfo);

    if (!bSuccess) {
        return {-2, 0.0};
    }

    CloseHandle(hChildStd_OUT_Wr);
    CloseHandle(hChildStd_IN_Rd);

    auto start_time = chrono::steady_clock::now();

    int gx = map.gollum.first, gy = map.gollum.second;
    string input = to_string(variant) + "\n" + to_string(gx) + " " + to_string(gy) + "\n";
    DWORD dwWritten;
    WriteFile(hChildStd_IN_Wr, input.c_str(), (DWORD)input.size(), &dwWritten, NULL);

    int r_perc = (variant == 1) ? 1 : 2;
    vector<Percept> percepts = get_percepts(0, 0, r_perc, false, false, map, false);
    input = to_string(percepts.size()) + "\n";
    for (const auto& p : percepts) {
        input += to_string(p.x) + " " + to_string(p.y) + " " + p.t + "\n";
    }
    WriteFile(hChildStd_IN_Wr, input.c_str(), (DWORD)input.size(), &dwWritten, NULL);

    int curx = 0, cury = 0;
    bool ring = false, mith = false, have_mount = false, reached_gollum = false;
    int moves = 0;

    string output_buffer;
    char buffer[1024];
    int cost = -2;
    bool done = false;

    while (!done) {
        DWORD dwRead;
        BOOL bRead = ReadFile(hChildStd_OUT_Rd, buffer, sizeof(buffer) - 1, &dwRead, NULL);
        if (!bRead || dwRead == 0) break;
        buffer[dwRead] = '\0';
        output_buffer += buffer;

        size_t pos;
        while ((pos = output_buffer.find('\n')) != string::npos) {
            string line = output_buffer.substr(0, pos);
            output_buffer = output_buffer.substr(pos + 1);

            if (line.empty()) continue;

            istringstream iss(line);
            string cmd;
            iss >> cmd;

            if (cmd == "m") {
                int nx, ny;
                iss >> nx >> ny;
                curx = nx;
                cury = ny;
                moves++;
                if (make_pair(curx, cury) == map.mithril) mith = true;
                if (make_pair(curx, cury) == map.gollum) reached_gollum = true;
            } else if (cmd == "r") {
                ring = true;
            } else if (cmd == "rr") {
                ring = false;
            } else if (cmd == "e") {
                iss >> cost;
                done = true;
                break;
            }

            // Send percepts
            percepts = get_percepts(curx, cury, r_perc, ring, mith, map, have_mount);
            input = to_string(percepts.size()) + "\n";
            for (const auto& p : percepts) {
                input += to_string(p.x) + " " + to_string(p.y) + " " + p.t + "\n";
            }
            if (reached_gollum && !have_mount && curx == gx && cury == gy) {
                have_mount = true;
                int mdx = map.mount.first, mdy = map.mount.second;
                input += to_string(mdx) + " " + to_string(mdy) + "\n";
            }
            WriteFile(hChildStd_IN_Wr, input.c_str(), (DWORD)input.size(), &dwWritten, NULL);
        }
    }

    double exec_time = chrono::duration<double>(chrono::steady_clock::now() - start_time).count();

    CloseHandle(hChildStd_IN_Wr);
    CloseHandle(hChildStd_OUT_Rd);
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    return {cost, exec_time};
}

void print_map(const MapData& map) {
    vector<vector<char>> grid(N, vector<char>(N, '.'));
    grid[0][0] = 'F';
    grid[map.gollum.first][map.gollum.second] = 'G';
    grid[map.mount.first][map.mount.second] = 'M';
    grid[map.mithril.first][map.mithril.second] = 'C';
    for (auto& e : map.enemies) {
        int ex, ey; char et;
        tie(ex, ey, et) = e;
        grid[ex][ey] = et;
    }
    cout << "Map:" << endl;
    for (auto& row : grid) {
        for (char c : row) cout << c << " ";
        cout << endl;
    }
    cout << endl;
}

struct Stats {
    double mean_time = 0.0;
    double median_time = 0.0;
    double mode_time = 0.0;
    double stdev_time = 0.0;
    int wins = 0;
    int losses = 0;
    double percent_win = 0.0;
    double percent_loss = 0.0;
};

Stats compute_stats(const vector<pair<int, double>>& res_list) {
    vector<int> costs;
    vector<double> times;
    for (auto& p : res_list) {
        if (p.first != -2) {
            costs.push_back(p.first);
            times.push_back(p.second);
        }
    }
    Stats stats;
    if (times.empty()) return stats;

    stats.wins = 0;
    stats.losses = 0;
    for (int c : costs) {
        if (c >= 0) ++stats.wins;
        else if (c == -1) ++stats.losses;
    }
    int n = times.size();
    stats.percent_win = 100.0 * stats.wins / n;
    stats.percent_loss = 100.0 * stats.losses / n;

    double sum = 0.0;
    for (double t : times) sum += t;
    stats.mean_time = sum / n;

    double var = 0.0;
    for (double t : times) var += (t - stats.mean_time) * (t - stats.mean_time);
    if (n > 1) stats.stdev_time = sqrt(var / (n - 1));
    else stats.stdev_time = 0.0;

    vector<double> sorted_times = times;
    sort(sorted_times.begin(), sorted_times.end());
    if (n % 2 == 1) stats.median_time = sorted_times[n / 2];
    else stats.median_time = (sorted_times[n / 2 - 1] + sorted_times[n / 2]) / 2.0;

    map<double, int> freq;
    for (double t : times) ++freq[t];
    int max_freq = 0;
    double mode = 0.0;
    for (auto& kv : freq) {
        if (kv.second > max_freq) {
            max_freq = kv.second;
            mode = kv.first;
        }
    }
    if (max_freq > 1) stats.mode_time = mode;
    else stats.mode_time = 0.0;

    return stats;
}

void print_stats(const string& key, const Stats& stats) {
    cout << "Stats for " << key << ":" << endl;
    cout << "mean_time: " << stats.mean_time << endl;
    cout << "median_time: " << stats.median_time << endl;
    cout << "mode_time: " << stats.mode_time << endl;
    cout << "stdev_time: " << stats.stdev_time << endl;
    cout << "wins: " << stats.wins << endl;
    cout << "losses: " << stats.losses << endl;
    cout << "percent_win: " << stats.percent_win << endl;
    cout << "percent_loss: " << stats.percent_loss << endl;
    cout << endl;
}

int main() {
    map<string, vector<pair<int, double>>> results;
    vector<MapData> impossible_maps;

    for (int i = 0; i < NUM_TESTS; ++i) {
        MapData map = generate_map();
        for (int variant : {1, 2}) {
            auto res_a = run_algo("astar", variant, map);
            auto res_b = run_algo("backtracking", variant, map);
            string key_a = "astar_v" + to_string(variant);
            string key_b = "back_v" + to_string(variant);
            results[key_a].push_back(res_a);
            results[key_b].push_back(res_b);
            if (res_a.first == -1 && res_b.first == -1) {
                impossible_maps.push_back(map);
            }
        }
    }

    for (auto& kv : results) {
        Stats stats = compute_stats(kv.second);
        print_stats(kv.first, stats);
    }

    cout << "\nImpossible maps:" << endl;
    for (const auto& imp_map : impossible_maps) {
        print_map(imp_map);
    }

    return 0;
}