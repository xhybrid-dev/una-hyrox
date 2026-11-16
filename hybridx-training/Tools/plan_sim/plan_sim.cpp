/**
 * @file    plan_sim.cpp
 * @brief   Print the plan-memory scenarios as Markdown (TP5's gate).
 *
 *     plan_sim            every scenario
 *     plan_sim 3          scenario 3 only (numbered as printed)
 */

#include "PlanSim.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace XTraining;

int main(int argc, char** argv)
{
    if (argc > 1) {
        const std::vector<SimScenario> all = scenarios();
        const size_t n = static_cast<size_t>(std::atoi(argv[1]));
        if (n < 1 || n > all.size()) {
            std::fprintf(stderr, "scenarios 1 to %zu\n", all.size());
            return 2;
        }
        std::fputs(runScenario(all[n - 1]).c_str(), stdout);
        return 0;
    }
    std::fputs(allScenarios().c_str(), stdout);
    return 0;
}
