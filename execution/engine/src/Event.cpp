#include "Event.h"

using namespace BPMNOS::Execution;

Event::Event(const Token* token)
  : token( token ? token->weak_from_this() : std::weak_ptr<const Token>() )
{
}

bool Event::expired() const {
  // A token-bearing event is stale once its token no longer exists.
  return token.expired();
}

