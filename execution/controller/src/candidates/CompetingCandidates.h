#ifndef BPMNOS_Execution_CompetingCandidates_H
#define BPMNOS_Execution_CompetingCandidates_H

#include <bpmn++.h>
#include <tuple>
#include <memory>
#include <utility>
#include <cstddef>
#include "execution/controller/src/Evaluator.h"
#include "SequentialEntries.h"
#include "MessageDeliveries.h"

namespace BPMNOS::Execution {

class Notifier;

/**
 * @brief Iterable union of the contested entry and message-delivery candidate decisions, ordered by reward.
 *
 * Owns a candidate collection of sequential-ad-hoc-subprocess child entries and one of message deliveries,
 * neither of which has precedence over the other. Iterating yields their two reward-ordered ranges as a single
 * reward-ordered range by a lazy two-way merge: at each step the better of the two fronts is yielded and that
 * side advanced, so the global best feasible decision is simply the front — without merging into a container or
 * re-sorting. Each yielded element carries just the reward, the decision's weak Event, and its weak Evaluation,
 * which is all a dispatcher needs to dispatch (and, for a rollout, to force) the decision.
 */
class CompetingCandidates {
public:
  CompetingCandidates(Evaluator* evaluator)
    : sequentialEntries(evaluator)
    , messageDeliveries(evaluator)
  {
  }

  /// Connect both owned candidate collections so each registers itself for the observables it needs.
  void connect(Notifier* notifier) {
    sequentialEntries.connect(notifier);
    messageDeliveries.connect(notifier);
  }

  /// Element yielded by the merge: reward, the decision's weak Event (second-to-last), its weak Evaluation (last).
  using value_type = std::tuple< double, std::weak_ptr<Event>, std::weak_ptr<Evaluation> >;

private:
  SequentialEntries sequentialEntries;
  MessageDeliveries messageDeliveries;

  using EntryIterator = decltype( std::declval<SequentialEntries&>().begin() );
  using MessageIterator = decltype( std::declval<MessageDeliveries&>().begin() );

  /// Project a candidate tuple of either collection onto (reward, weak Event, weak Evaluation).
  template <typename Tuple>
  static value_type project(const Tuple& candidate) {
    constexpr std::size_t size = std::tuple_size_v<Tuple>;
    return value_type{ std::get<0>(candidate), std::get<size - 2>(candidate), std::get<size - 1>(candidate) };
  }

public:
  /// End marker for the merge.
  struct Sentinel {};

  /// Lazy two-way merge of the two reward-ordered ranges; yields the better front, advances that side.
  class Iterator {
  public:
    Iterator(EntryIterator entry, EntryIterator entryEnd, MessageIterator message, MessageIterator messageEnd)
      : entry(entry), entryEnd(entryEnd), message(message), messageEnd(messageEnd)
    {
    }

    bool operator!=(Sentinel) const { return entry != entryEnd || message != messageEnd; }

    value_type operator*() const { return preferEntry() ? project(*entry) : project(*message); }

    Iterator& operator++() {
      if ( preferEntry() ) ++entry; else ++message;
      return *this;
    }

  private:
    /// Yield the entry front while it exists and is at least as good as the message front (reward-descending).
    bool preferEntry() const {
      if ( !(entry != entryEnd) ) return false;
      if ( !(message != messageEnd) ) return true;
      return std::get<0>(*entry) >= std::get<0>(*message);
    }

    EntryIterator entry, entryEnd;
    MessageIterator message, messageEnd;
  };

  /// Iterating triggers each collection's lazy evaluation (via its begin) and merges the results.
  Iterator begin() {
    return Iterator( sequentialEntries.begin(), sequentialEntries.end(),
                     messageDeliveries.begin(), messageDeliveries.end() );
  }
  Sentinel end() { return {}; }
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CompetingCandidates_H
