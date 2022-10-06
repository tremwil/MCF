
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include <queue>
#include <unordered_set>
#include "AhoCorasickScanner.h"

namespace MCF
{
    void AhoCorasickScanner::BuildStateMachine()
    {
        // Implementation based on https://www.geeksforgeeks.org/aho-corasick-algorithm-pattern-searching/
        // Adapted for arbitrary keyword count and byte string searching

        num_states = 0;
        max_states = 0; // Maximum amount of states the algorithm could require
        for (const auto& s : keywords)
            max_states += s.size();

        transition_fun.clear();
        transition_fun.resize(256 * max_states, -1);

        failure_fun.clear();
        failure_fun.resize(max_states, -1);

        output_fun.clear();
        output_fun.resize(max_states);

        // Only required so that algorithm time complexity is not affected by merging output functions
        std::vector<std::unordered_set<int>> temp_output(max_states);

        for (int i = 0; i < keywords.size(); i++) {
            const auto& word = keywords[i];
            int curr_state = 0;

            // Insert bytes into DFA
            for (auto c: word) {
                auto idx = 256 * curr_state + (uint8_t)c;
                if (transition_fun[idx] == -1)
                    transition_fun[idx] = num_states++;

                curr_state = transition_fun[idx];
            }

            // Add word to output function for given state
            temp_output[curr_state].insert(i);
        }

        // loop initial state for bytes that aren't at the start of any keyword
        for (int c = 0; c < 256; c++) {
            if (transition_fun[c] == -1) transition_fun[c] = 0;
        }

        // Build failure transitions using breadth first search
        std::queue<int> bfs_queue;

        for (int c = 0; c < 256; c++) {
            if (transition_fun[c] != 0) {
                failure_fun[transition_fun[c]] = 0;
                bfs_queue.push(transition_fun[c]);
            }
        }
        while (!bfs_queue.empty()) {
            int state = bfs_queue.front();
            bfs_queue.pop();

            for (int c = 0; c < 256; c++) {
                int idx = 256 * state + c, t = transition_fun[idx];
                if (t != -1) {
                    int f = failure_fun[state];
                    while (transition_fun[256 * f + c] == -1)
                        f = failure_fun[f];

                    f = transition_fun[256 * f + c];
                    failure_fun[t] = f;

                    for (auto out : temp_output[f])
                        temp_output[t].insert(out);

                    bfs_queue.push(t);
                }
            }
        }

        // Build vector based output function from temp_output maps
        for (int i = 0; i < num_states; i++) {
            for (auto out : temp_output[i])
                output_fun[i].push_back(out);
        }
    }
} // MCF