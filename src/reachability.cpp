#include <vector>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <iostream>
#include <bitset>
#include <functional>
#include <list>
#include <numeric>

#include "BooleanNetwork.h"
#include "SolutionObjects.h"

using namespace std;

// Helper function to generate all combinations
vector<vector<int>> generate_combinations(int n) {
    vector<vector<int>> result;
    for(int i=0; i<(1<<n); ++i) {
        vector<int> combo;
        for(int j=0; j<n; ++j) {
            combo.push_back((i >> j) & 1);
        }
        result.push_back(combo);
    }
    return result;
}

vector<int> get_nodes_to_explore(const BooleanNetwork& network,
                               const map<int, vector<pair<map<int, int>, int>>>& exploration_functions,
                               const set<int>& not_included_nodes) {
    set<int> state_to_explore;
    for(const auto& [k, v] : exploration_functions) {
        if(not_included_nodes.count(k)) {
            for(const auto& [f, _] : v) {
                for(const auto& [i, _] : f) {
                    state_to_explore.insert(i);
                }
            }
        }
    }

    bool updated = true;
    while(updated) {
        updated = false;
        vector<int> to_add;
        for(int node_id : state_to_explore) {
            for(const auto& [f, _] : exploration_functions.at(node_id)) {
                for(const auto& [k, _] : f) {
                    if(!state_to_explore.count(k)) {
                        to_add.push_back(k);
                        updated = true;
                    }
                }
            }
        }
        for(int x : to_add) state_to_explore.insert(x);
    }

    vector<int> filtered;
    copy_if(state_to_explore.begin(), state_to_explore.end(), back_inserter(filtered),
        [&network](int i) { return i < network.state_size; });

    cout << "state_to_explore: " << filtered.size() << endl;
    return filtered;
}

vector<vector<int>> get_included_solutions_states(const TrapSpace& included_solution,
                                                const vector<int>& state_to_explore) {
    vector<vector<int>> result(1, vector<int>(state_to_explore.size(), -1));

    for(size_t i=0; i<state_to_explore.size(); ++i) {
        int k = state_to_explore[i];
        auto it = included_solution.stable_nodes.find(k);
        if(it != included_solution.stable_nodes.end()) {
            result[0][i] = it->second;
        }
    }

    for(size_t i=0; i<state_to_explore.size(); ++i) {
        int k = state_to_explore[i];
        if(!included_solution.stable_nodes.count(k)) {
            vector<vector<int>> new_result;
            for(auto& row : result) {
                vector<int> row0 = row;
                row0[i] = 0;
                vector<int> row1 = row;
                row1[i] = 1;
                new_result.push_back(row0);
                new_result.push_back(row1);
            }
            result = move(new_result);
        }
    }
    return result;
}

int check_if_reachable(const TrapSpace& solution,
                      const vector<TrapSpace>& included_solutions,
                      const map<int, vector<pair<map<int, int>, int>>>& exploration_functions) {
    vector<int> state_to_explore;
    for(const auto& [k, _] : exploration_functions) {
        state_to_explore.push_back(k);
    }
    sort(state_to_explore.begin(), state_to_explore.end());

    if(state_to_explore.size() > 23) {
        cout << "state_to_explore: " << state_to_explore.size() << endl;
        return -1;
    }

    map<int, vector<vector<int>>> func;
    map<int, vector<int>> tau;
    for(const auto& [k, funcs] : exploration_functions) {
        if(find(state_to_explore.begin(), state_to_explore.end(), k) != state_to_explore.end()) {
            for(const auto& [f, t] : funcs) {
                vector<int> row;
                for(int v : state_to_explore) {
                    row.push_back(f.count(v) ? f.at(v) : 0);
                }
                func[k].push_back(row);
                tau[k].push_back(t);
            }
        }
    }

    queue<vector<int>> exploration_queue;
    map<vector<int>, bool> exploration_states;
    auto all_combos = generate_combinations(state_to_explore.size());
    for(const auto& combo : all_combos) {
        exploration_states[combo] = false;
    }

    int reachable_states = 0;
    for(const auto& s : included_solutions) {
        auto states = get_included_solutions_states(s, state_to_explore);
        for(const auto& state : states) {
            exploration_queue.push(state);
            exploration_states[state] = true;
            reachable_states++;
        }
    }

    while(!exploration_queue.empty()) {
        auto state = exploration_queue.front();
        exploration_queue.pop();

        // Generate neighbor states
        vector<vector<int>> neighbors;
        for(size_t i=0; i<state.size(); ++i) {
            auto neighbor = state;
            neighbor[i] = 1 - neighbor[i];
            neighbors.push_back(neighbor);
        }

        // Check reachability
        for(auto& neighbor : neighbors) {
            bool valid = true;
            for(size_t i=0; i<state_to_explore.size(); ++i) {
                int k = state_to_explore[i];
                int sum = inner_product(
                    func[k].begin(), func[k].end(),
                    neighbor.begin(), 0
                );
                if(sum < tau[k][i]) {
                    valid = false;
                    break;
                }
            }

            if(valid && !exploration_states[neighbor]) {
                exploration_states[neighbor] = true;
                exploration_queue.push(neighbor);
                reachable_states++;
            }
        }

        if(reachable_states == pow(2, state_to_explore.size())) break;
    }

    if(reachable_states < all_combos.size())
    {
        return exploration_states.size() - reachable_states;
    }
    return 0;
}

// Similar implementations for get_reduced_threshold_functions and get_trivial_nodes
// would follow the same patterns using STL containers and algorithms

map<int, vector<map<int, int>>> get_trivial_nodes(const map<int, vector<map<int, int>>>& f) {
    // Create node-to-parents mapping
    map<int, set<int>> node_to_parents;
    for (const auto& [k, val] : f) {
        set<int> parents;
        for (const auto& func : val) {
            for (const auto& [p, _] : func) {
                parents.insert(p);
            }
        }
        node_to_parents[k] = parents;
    }

    // Create node-to-children mapping
    map<int, vector<int>> node_to_children;
    for (const auto& [k, _] : f) {
        node_to_children[k] = vector<int>();
    }
    for (const auto& [k, parents] : node_to_parents) {
        for (int parent : parents) {
            node_to_children[parent].push_back(k);
        }
    }

    map<int, vector<map<int, int>>> new_f = f;
    bool updated = true;

    while (updated) {
        updated = false;
        list<int> nodes_to_process;
        for (const auto& [node_id, _] : node_to_children) {
            nodes_to_process.push_back(node_id);
        }

        for (int node_id : nodes_to_process) {
            if (!node_to_children.count(node_id)) continue;

            auto& children = node_to_children[node_id];

            if (children.size() > 1) continue;

            if (children.empty()) {
                // Remove node with no children
                updated = true;
                node_to_children.erase(node_id);
                new_f.erase(node_id);

                if (node_to_parents.count(node_id)) {
                    auto parents = node_to_parents[node_id];
                    node_to_parents.erase(node_id);
                    for (int parent : parents) {
                        auto& parent_children = node_to_children[parent];
                        parent_children.erase(
                            remove(parent_children.begin(), parent_children.end(), node_id),
                            parent_children.end()
                        );
                    }
                }
            }
            else if (node_to_parents.count(node_id) && node_to_parents[node_id].size() == 1) {
                // Check if we can merge nodes
                int parent = *node_to_parents[node_id].begin();
                int child = children[0];

                if (find(node_to_children[parent].begin(), node_to_children[parent].end(), node_id)
                    == node_to_children[parent].end()) {
                    continue;
                }

                // Check weights (simplified check)
                bool sufficient_weight = true;
                if (!new_f[node_id].empty() && new_f[node_id][0].count(parent)) {
                    if (new_f[node_id][0][parent] < 1) sufficient_weight = false;
                }
                if (!new_f[child].empty() && new_f[child][0].count(node_id)) {
                    if (new_f[child][0][node_id] < 1) sufficient_weight = false;
                }

                if (sufficient_weight) {
                    updated = true;

                    // Update parent-child relationships
                    node_to_children[parent].erase(
                        remove(node_to_children[parent].begin(), node_to_children[parent].end(), node_id),
                        node_to_children[parent].end()
                    );
                    node_to_children[parent].push_back(child);

                    // Update functions
                    for (auto& func : new_f[child]) {
                        if (func.count(node_id)) {
                            int weight = func[node_id];
                            func.erase(node_id);
                            func[parent] += weight;
                        }
                    }

                    // Cleanup old node
                    node_to_children.erase(node_id);
                    node_to_parents.erase(node_id);
                    new_f.erase(node_id);
                }
            }
        }
    }

    return new_f;
}

map<int, vector<pair<map<int, int>, int>>> get_reduced_threshold_functions(
    BooleanNetwork& network,
    const map<int, int>& stable_nodes,
    const vector<int>& external,
    const vector<TrapSpace>& included_solutions) {

    // Step 1: Filter functions and thresholds
    map<int, vector<map<int, int>>> functions;
    map<int, vector<int>> thresholds;

    for (const auto& [k, v] : network.get_threshold_functions()) {
        if (k < network.state_size && !stable_nodes.count(k)) {
            vector<map<int, int>> funcs;
            vector<int> thresh;
            for (const auto& [f, t] : v) {
                funcs.push_back(f);
                thresh.push_back(t);
            }
            functions[k] = funcs;
            thresholds[k] = thresh;
        }
    }

    // Step 2: Calculate default values
    map<int, vector<int>> default_states_values;
    for (const auto& [k, funcs] : functions) {
        vector<int> values;
        for (const auto& f : funcs) {
            int sum = 0;
            for (const auto& [i, weight] : f) {
                sum += stable_nodes.count(i) ? stable_nodes.at(i) * weight : 0;
            }
            values.push_back(sum);
        }
        default_states_values[k] = values;
    }

    map<int, vector<int>> defaults_external_values;
    for (const auto& [k, funcs] : functions) {
        vector<int> values;
        for (const auto& f : funcs) {
            int sum = 0;
            for (const auto& [i, weight] : f) {
                if (i >= network.state_size) {
                    int ext_idx = i - network.state_size;
                    sum += external[ext_idx] * weight;
                }
            }
            values.push_back(sum);
        }
        defaults_external_values[k] = values;
    }

    // Step 3: Update thresholds
    map<int, vector<int>> updated_threshold;
    for (const auto& [k, thresh] : thresholds) {
        vector<int> new_thresh;
        for (size_t j = 0; j < thresh.size(); ++j) {
            new_thresh.push_back(thresh[j]
                - default_states_values[k][j]
                - defaults_external_values[k][j]);
        }
        updated_threshold[k] = new_thresh;
    }

    // Step 4: Update functions
    map<int, vector<map<int, int>>> updated_functions;
    for (const auto& [k, funcs] : functions) {
        vector<map<int, int>> new_funcs;
        for (const auto& f : funcs) {
            map<int, int> new_f;
            for (const auto& [i, weight] : f) {
                if (!stable_nodes.count(i) && i < network.state_size) {
                    new_f[i] = weight;
                }
            }
            new_funcs.push_back(new_f);
        }
        updated_functions[k] = new_funcs;
    }

    // Step 5: Simplify functions
    auto new_updated_functions = get_trivial_nodes(updated_functions);

    // Step 6: Combine functions and thresholds
    map<int, vector<pair<map<int, int>, int>>> updated_thresholds_func;
    for (const auto& [i, funcs] : new_updated_functions) {
        vector<pair<map<int, int>, int>> combined;
        for (size_t j = 0; j < funcs.size(); ++j) {
            combined.emplace_back(funcs[j], updated_threshold[i][j]);
        }
        updated_thresholds_func[i] = combined;
    }

    // Step 7: Determine nodes to explore
    set<int> not_included_nodes;
    for (const auto& s : included_solutions) {
        for (const auto& [i, _] : s.stable_nodes) {
            if (!stable_nodes.count(i)) {
                not_included_nodes.insert(i);
            }
        }
    }

    auto nodes_to_explore = get_nodes_to_explore(network, updated_thresholds_func,
        set<int>(not_included_nodes.begin(), not_included_nodes.end()));

    // Final filtering
    map<int, vector<pair<map<int, int>, int>>> result;
    for (const auto& [i, v] : updated_thresholds_func) {
        if (find(nodes_to_explore.begin(), nodes_to_explore.end(), i) != nodes_to_explore.end()) {
            result[i] = v;
        }
    }

    return result;
}


