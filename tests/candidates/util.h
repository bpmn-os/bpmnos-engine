#pragma once

#include <limits>
#include <set>
#include <string>
#include <vector>

// Shared helpers for the candidate-collection tests: turn a candidate collection into a list of
// (reward, identity) records, so a rebuilt set can be compared against an incrementally built one.

struct CandidateInfo {
  double reward;
  std::string tag;   ///< stable identity: "<instanceId>@<nodeId>"
};

inline std::string tokenTag(const BPMNOS::Execution::Token* token) {
  if ( !token ) return "null";
  std::string node = token->node ? token->node->id : std::string("(process)");
  return std::to_string((double)token->getInstanceId()) + "@" + node;
}

// Collect candidates whose tuple carries the token as the second element (entry/exit/choice/message deliveries).
template <typename Candidates>
std::vector<CandidateInfo> collect(Candidates& candidates) {
  std::vector<CandidateInfo> result;
  for ( auto candidate : candidates ) {
    auto token = std::get<1>(candidate).lock();
    result.push_back({ std::get<0>(candidate), tokenTag(token.get()) });
  }
  return result;
}

// CompetingCandidates yields (reward, weak Event, weak Evaluation); recover the token via the decision's Event.
inline std::vector<CandidateInfo> collectCompeting(BPMNOS::Execution::CompetingCandidates& candidates) {
  std::vector<CandidateInfo> result;
  for ( auto candidate : candidates ) {
    auto event = std::get<1>(candidate).lock();
    std::shared_ptr<const BPMNOS::Execution::Token> token;
    if ( auto decision = std::dynamic_pointer_cast<BPMNOS::Execution::Decision>(event) ) {
      token = decision->token.lock();
    }
    result.push_back({ std::get<0>(candidate), tokenTag(token.get()) });
  }
  return result;
}

inline std::multiset<std::string> signatures(const std::vector<CandidateInfo>& candidates) {
  std::multiset<std::string> result;
  for ( auto& candidate : candidates ) {
    result.insert( std::to_string(candidate.reward) + "|" + candidate.tag );
  }
  return result;
}

inline std::size_t feasibleCount(const std::vector<CandidateInfo>& candidates) {
  std::size_t count = 0;
  for ( auto& candidate : candidates ) {
    if ( candidate.reward > -std::numeric_limits<double>::infinity() ) ++count;
  }
  return count;
}

inline bool rewardDescending(const std::vector<CandidateInfo>& candidates) {
  for ( std::size_t i = 1; i < candidates.size(); ++i ) {
    if ( candidates[i].reward > candidates[i-1].reward ) return false;
  }
  return true;
}
