#ifndef SOLUTION_OBJECTS_H
#define SOLUTION_OBJECTS_H

#include <map>
#include <vector>
#include <string>
#include <stdexcept>

class TrapSpace {
public:
    int solution_id;
    std::map<int, int> stable_nodes;
    bool included_solution = false;

    TrapSpace(int id, const std::map<int, int>& nodes)
        : solution_id(id), stable_nodes(nodes) {}

    void mark_as_included_solution()
    {
        included_solution = true;
    }
};

class SolutionObjects {
public:
    std::map<int, TrapSpace> solutions;
    std::map<int, std::vector<std::vector<int>>> external_assignments;
    std::map<int, int> solution_to_externals;
    std::map<int, std::vector<int>> all_included_solutions;
    std::map<std::pair<int, int>, std::vector<std::vector<int>>> all_included_externals;
    int solution_id_counter;
    int external_id_counter;

    SolutionObjects();

    int add_solution(const std::map<int, int>& stable_nodes);
    void remove_solution(int solution_id);
    void remove_external(int external_id);
    void update_external_to_solution(int solution_id, int externals_id);
    int add_externals_assignments(const std::vector<std::vector<int>>& externals_assignments);
    std::vector<int> get_not_null_externals_for_solution(int solution_id);
    std::vector<std::vector<int>> get_external_assignment_for_solution(int solution_id);
};

#endif // SOLUTION_OBJECTS_H
