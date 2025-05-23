#ifndef _BPMNOS_Execution_Notifier_H
#define _BPMNOS_Execution_Notifier_H

#include <vector>
#include <list>
#include "Observable.h"
#include "Observer.h"

namespace BPMNOS::Execution {

class Notifier {
public:
  Notifier();
  virtual ~Notifier() = default;
  
  template<typename... ObservableTypes>
  void addSubscriber(Observer* subscriber, ObservableTypes... observableTypes)  {
    (subscribers[(size_t)observableTypes].push_back(subscriber), ...);
  }

  template<typename... ObservableTypes>
  void removeSubscriber(Observer* subscriber, ObservableTypes... observableTypes)  {
    (subscribers[(size_t)observableTypes].remove(subscriber), ...);
  }

  void notify(const Observable* observable) const;
  void notify(const Observable& observable) const;
private:
  std::vector< std::list<Observer*> > subscribers;
};

} // namespace BPMNOS::Execution
#endif // _BPMNOS_Execution_Notifier_H
