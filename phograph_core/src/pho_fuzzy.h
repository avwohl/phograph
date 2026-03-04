#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace pho {

struct FuzzyMatch {
    std::string text;
    int score = 0;     // higher is better
    int index = 0;     // original index in the list
};

// Score a single query against a candidate string.
// Returns a score >= 0. Higher is better. 0 means no match.
inline int fuzzy_score(const std::string& query, const std::string& text) {
    if (query.empty()) return 1; // empty query matches everything
    size_t qi = 0;
    int score = 0;
    bool prev_match = false;

    for (size_t ti = 0; ti < text.size() && qi < query.size(); ti++) {
        char qc = query[qi] >= 'A' && query[qi] <= 'Z' ? query[qi] + 32 : query[qi];
        char tc = text[ti] >= 'A' && text[ti] <= 'Z' ? text[ti] + 32 : text[ti];
        if (qc == tc) {
            qi++;
            score += 10;
            // Bonus for consecutive matches
            if (prev_match) score += 5;
            // Bonus for match at start of word
            if (ti == 0 || text[ti - 1] == '-' || text[ti - 1] == '_' || text[ti - 1] == ' ' ||
                (text[ti] >= 'A' && text[ti] <= 'Z'))
                score += 8;
            prev_match = true;
        } else {
            prev_match = false;
        }
    }

    if (qi < query.size()) return 0; // Not all query chars matched
    // Bonus for shorter text (tighter match)
    score += std::max(0, 50 - static_cast<int>(text.size()));
    return score;
}

// Filter and rank a list of candidates by fuzzy query.
// Returns matches sorted by score (descending), limited to max_results.
inline std::vector<FuzzyMatch> fuzzy_filter(
    const std::string& query,
    const std::vector<std::string>& candidates,
    int max_results = 20
) {
    std::vector<FuzzyMatch> matches;
    for (size_t i = 0; i < candidates.size(); i++) {
        int s = fuzzy_score(query, candidates[i]);
        if (s > 0) {
            matches.push_back({candidates[i], s, static_cast<int>(i)});
        }
    }
    std::sort(matches.begin(), matches.end(),
              [](const FuzzyMatch& a, const FuzzyMatch& b) { return a.score > b.score; });
    if (static_cast<int>(matches.size()) > max_results) {
        matches.resize(max_results);
    }
    return matches;
}

} // namespace pho
