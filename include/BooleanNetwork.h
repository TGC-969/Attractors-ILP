#ifndef BOOLEAN_NETWORK_H
#define BOOLEAN_NETWORK_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include "Node.h"

using ThresholdFunction = std::pair<std::vector<int>, int>;
class BooleanNetwork {
public:
    explicit BooleanNetwork(const std::string& network_name, const std::string& path = "");

    std::unordered_map<int, ThresholdFunction> get_threshold_functions();
    void display_network_threshold_function();
    int get_state_size() const;

    int edges = 0;
    int size = 0;
    int state_size = 0;
    int external_size = 0;
    std::string name;

    std::vector<std::string> state_nodes_names;
    std::vector<std::string> external_nodes_names;
    std::vector<std::string> external_only_depended_nodes_names;
    std::vector<std::string> hole_nodes_names;
    std::vector<std::string> index_to_name;
    std::vector<std::string> combined_nodes_names; // state_nodes_names + external_only_depended_nodes_names + hole_nodes_names;
    std::unordered_map<std::string, int> nameToId;
    std::vector<std::string> get_updated_successors_for_node(const std::string& name) const;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;

    std::unordered_map<int, ThresholdFunction> threshold_functions;

private:
    void updated_network();
    void delete_not_influence_nodes();
    std::vector<std::string> delete_external_only_depended();
    std::vector<std::string> delete_hole_nodes();

     // Placeholder – actual serialization not implemented







    std::unordered_map<int, std::unordered_map<std::string, std::vector<int>>> unate_dict;
};

#endif

