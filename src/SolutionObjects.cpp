#include "SolutionObjects.h"
#include <algorithm>
#include <iostream>

SolutionObjects::SolutionObjects() 
    : solution_id_counter(0), external_id_counter(0) {}

int SolutionObjects::add_solution(const std::map<int, int>& stable_nodes) {
    int current_id = solution_id_counter++;
    solutions.emplace(current_id, TrapSpace(current_id, stable_nodes));
    return current_id;
}

void SolutionObjects::remove_solution(int solution_id) {
    solutions.erase(solution_id);
    solution_to_externals.erase(solution_id);
}

void SolutionObjects::remove_external(int external_id) {
    // Check if any solutions are using this external ID
    std::vector<int> solutions_using;
    for(const auto& pair : solution_to_externals) {
        if(pair.second == external_id) {
            solutions_using.push_back(pair.first);
        }
    }
    
    if(!solutions_using.empty()) {
        std::cerr << "Error: " << external_id << " has " << solutions_using.size() 
                  << " related solutions.\n";
        for(int sid : solutions_using) std::cerr << sid << " ";
        std::cerr << std::endl;
        return;
    }
    
    external_assignments.erase(external_id);
}

void SolutionObjects::update_external_to_solution(int solution_id, int externals_id) {
    solution_to_externals[solution_id] = externals_id;
}

int SolutionObjects::add_externals_assignments(const std::vector<std::vector<int>>& externals_assignments) {
    int new_id = external_id_counter++;
    this->external_assignments[new_id] = externals_assignments;
    return new_id;
}

std::vector<int> SolutionObjects::get_not_null_externals_for_solution(int solution_id) {
    std::vector<int> result;
    auto it = solution_to_externals.find(solution_id);
    if(it == solution_to_externals.end()) {
        throw std::runtime_error("Solution ID not found");
    }
    
    const auto& assignments = external_assignments[it->second];
    if(assignments.empty() || assignments[0].empty()) return result;
    
    for(size_t i = 0; i < assignments[0].size(); ++i) {
        if(assignments[0][i] != -1) {
            result.push_back(i);
        }
    }
    return result;
}

std::vector<std::vector<int>> SolutionObjects::get_external_assignment_for_solution(int solution_id) {
    auto ext_it = solution_to_externals.find(solution_id);
    if(ext_it == solution_to_externals.end()) {
        throw std::runtime_error("Solution ID not found");
    }
    
    auto assign_it = external_assignments.find(ext_it->second);
    if(assign_it == external_assignments.end()) {
        throw std::runtime_error("External assignment not found");
    }
    
    return assign_it->second;
}