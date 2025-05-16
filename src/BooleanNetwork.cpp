#include "BooleanNetwork.h"
#include "expressionparser.h"
#include "Node.h"
#include <symengine/basic.h>

BooleanNetwork::BooleanNetwork(const std::string& network_name, const std::string& path)
{
    parseExpressions(network_name, nodes, nameToId);
    for (auto node : nodes)
    {
        if (node.second->external)
        {
            external_nodes_names.push_back(node.first);
        }
        else
        {
            state_nodes_names.push_back(node.first);
        }
        state_size++;
    }
    updated_network();
    get_threshold_functions();
}


std::unordered_map<int, ThresholdFunction> BooleanNetwork::get_threshold_functions()
{
    for(const std::string& node_name : combined_nodes_names)
    {
        auto node = nodes[node_name];
        threshold_functions[node->id] = node->threshold;
    }
    return threshold_functions;
}

void BooleanNetwork::updated_network()
{
    index_to_name.clear();
    state_size = 0;
    external_size = 0;
    delete_not_influence_nodes();

    for(const auto& node_name : state_nodes_names)
    {
        auto node = nodes[node_name];
        node->id = state_size;
        index_to_name[node->id] = node_name;
        state_size++;
    }

    for(const auto& node_name : external_nodes_names)
    {
        nodes[node_name]->id = external_size + state_size;
        index_to_name[nodes[node_name]->id] = node_name;
        external_size++;
    }

    std::vector<std::string> sorted_hole_nodes_names = hole_nodes_names;
    std::sort(sorted_hole_nodes_names.begin(), sorted_hole_nodes_names.end());
    for(const auto& node_name : sorted_hole_nodes_names)
    {
        auto node = nodes[node_name];
        bool isHole = true;
        auto parents = node->getParents();
        for(const auto& parent : parents)
        {
            if(std::find(state_nodes_names.begin(), state_nodes_names.end(), parent) == state_nodes_names.end())
            {
                isHole = false;
                break;
            }
        }
        if(isHole)
        {
            external_nodes_names.push_back(node_name);
            auto it = std::find(hole_nodes_names.begin(), hole_nodes_names.end(), node_name);
            if(it != hole_nodes_names.end()) hole_nodes_names.erase(it);
        }
    }

    int index = state_size + external_size;
    for(const auto& name : external_only_depended_nodes_names)
    {
        auto node = nodes[name];
        node->id = index;
        index_to_name[node->id] = name;
        index++;
    }

    for(const auto& node_name : hole_nodes_names)
    {
        auto node = nodes[node_name];
        node->id = index;
        index_to_name[node->id] = node_name;
        index++;
    }
}

void BooleanNetwork::delete_not_influence_nodes()
{
    while(true)
    {
        auto new_external_dependent = delete_external_only_depended();
        if(new_external_dependent.empty()) break;
    }
    if(size > 300)
    {
        while(true)
        {
            auto new_hole = delete_hole_nodes();
            if(new_hole.empty()) break;
        }
    }
}

std::vector<std::string> BooleanNetwork::delete_hole_nodes()
{
    std::vector<std::string> new_holes;

    for (const std::string& name : state_nodes_names) {
        std::vector<std::string> state_successors;
        for (const std::string& s : get_updated_successors_for_node(name)) {
            if (std::find(state_nodes_names.begin(), state_nodes_names.end(), s) != state_nodes_names.end()) {
                state_successors.push_back(s);
            }
        }
        if (state_successors.empty()) {
            new_holes.push_back(name);
        }
    }

    for (const std::string& name : new_holes) {
        std::map<std::string, SymEngine::RCP<const SymEngine::Basic>> successors_assignment = {
            {name, nodes[name]->boolean_function}
        };

        for (const std::string& s : get_updated_successors_for_node(name)) {
            nodes[s]->update_boolean_function(successors_assignment);
        }

        // Remove name from state_nodes_names
        state_nodes_names.erase(std::remove(state_nodes_names.begin(), state_nodes_names.end(), name), state_nodes_names.end());

        const std::vector<std::string>& parents = nodes[name]->getParents();
        bool all_parents_external = std::all_of(parents.begin(), parents.end(),
            [this](const std::string& p) {
                return std::find(external_nodes_names.begin(), external_nodes_names.end(), p) != external_nodes_names.end();
            });

        if (all_parents_external || parents.empty()) {
            external_nodes_names.push_back(name);
        } else {
            hole_nodes_names.push_back(name);
        }
    }

    return new_holes;
}

std::vector<std::string> BooleanNetwork::delete_external_only_depended()
{
    std::vector<std::string> external_only_depended_node;

        for (const auto& name : state_nodes_names) {
            auto node = nodes.at(name);

            // Filter parents that are in state_nodes_names
            std::vector<std::string> state_parents;
            for (const auto& p : node->getParents()) {
                if (std::find(state_nodes_names.begin(), state_nodes_names.end(), p) != state_nodes_names.end()) {
                    state_parents.push_back(p);
                }
            }

            if (!state_parents.empty()) {
                continue;
            }

            // node_parents = parents in external_nodes_names + state_parents (which is empty here)
            std::vector<std::string> node_parents;
            for (const auto& p : node->getParents()) {
                if (std::find(external_nodes_names.begin(), external_nodes_names.end(), p) != external_nodes_names.end()) {
                    node_parents.push_back(p);
                }
            }
            node_parents.insert(node_parents.end(), state_parents.begin(), state_parents.end());

            // Get successors in state_nodes_names
            auto state_successors = get_updated_successors_for_node(name);
            state_successors.erase(std::remove_if(state_successors.begin(), state_successors.end(),
                [this](const std::string& s) {
                    return std::find(state_nodes_names.begin(), state_nodes_names.end(), s) == state_nodes_names.end();
                }), state_successors.end());

            bool to_add = true;

            for (const auto& s : state_successors) {
                auto successor_parents = nodes.at(s)->getParents();

                // Combine successor_parents and node_parents uniquely
                std::unordered_set<std::string> combined_parents(successor_parents.begin(), successor_parents.end());
                combined_parents.insert(node_parents.begin(), node_parents.end());

                if (combined_parents.size() > 16) {
                    to_add = false;
                    break;
                }

                // Check if any p in node_parents is in successor_parents
                for (const auto& p : node_parents) {
                    if (std::find(successor_parents.begin(), successor_parents.end(), p) != successor_parents.end()) {
                        to_add = false;
                        break;
                    }
                }

                if (!to_add) break;
            }

            if (to_add) {
                external_only_depended_node.push_back(name);
            }
        }

        for (const auto& name : external_only_depended_node) {
            if (state_nodes_names.size() == 1) {
                external_only_depended_node.clear();
                break;
            }

            // successors_assignment: map from node name to its boolean function
            std::map<std::string, decltype(nodes.begin()->second->boolean_function)> successors_assignment;
            successors_assignment[name] = nodes.at(name)->boolean_function;

            for (const auto& s : get_updated_successors_for_node(name)) {
                nodes.at(s)->update_boolean_function(successors_assignment);
            }

            // Remove from state_nodes_names
            state_nodes_names.erase(std::remove(state_nodes_names.begin(), state_nodes_names.end(), name), state_nodes_names.end());

            // Add to external_only_depended_nodes_names
            external_only_depended_nodes_names.push_back(name);
        }

        return external_only_depended_node;
}

std::vector<std::string> BooleanNetwork::get_updated_successors_for_node(const std::string& name) const
{
    std::vector<std::string> successors_name;
    for(auto node : nodes)
    {
        const auto& parents = node.second->getParents();
        if(std::find(parents.begin(), parents.end(), name) != parents.end())
        {
            successors_name.push_back(name);
        }
    }
    return successors_name;
}



