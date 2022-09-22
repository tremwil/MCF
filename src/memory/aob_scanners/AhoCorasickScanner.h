
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace MCF
{
    class AhoCorasickScanner
    {
    private:
        size_t max_states = 0; // should be sum(aho_keywords total substring lengths)
        int num_states = 0;

        // DFA transition function indexed by 256*state + c
        std::vector<int> transition_fun;

        // Function pointing to next state in case of match failure
        std::vector<int> failure_fun;

        // Stores list of matched keywords by index for a given state
        std::vector<std::vector<int>> output_fun;

    public:
        std::vector<std::string> keywords;

        AhoCorasickScanner() = default;

        /// Set up the Aho-Corasick state machine with the given set of keywords.
        /// This must be called before initiating a scan.
        void BuildStateMachine(const std::vector<std::string>& kw);

        /// Search the given memory for matches with the registered keywords, firing the provided callback for each match.
        /// \param memory Pointer to memory region to scan.
        /// \param size Size of the memory region to scan.
        /// \param callback Callback called for every match with the index of the match in "keywords" and the start address. Can return true to abort the scan.
        template<class TCallback> requires std::is_invocable_r_v<bool, TCallback, int, uintptr_t>
        void Search(const uint8_t* memory, size_t size, const TCallback& callback) const;
    };

} // MCF