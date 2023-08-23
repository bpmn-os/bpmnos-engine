#ifndef BPMNOS_Token_H
#define BPMNOS_Token_H

namespace BPMNOS {

class Token {
public:
  Token();

  bool ready() const { return state == READY; };
  bool waiting() const { return state == WAITING; };
  bool entered() const { return state == ENTERED; };
  bool busy() const { return state == BUSY; };
  bool completed() const { return state == COMPLETED; };
  bool departed() const { return state == DEPARTED; };
  bool arrived() const { return state == ARRIVED; };
  bool done() const { return state == DONE; };
  bool failed() const { return state == FAILED; };
private:
  enum State { CREATED, READY, WAITING, ENTERED, BUSY, COMPLETED, DEPARTED, ARRIVED, DONE, FAILED, TO_BE_MERGED, TO_BE_COPIED };
  State state;
};

} // namespace BPMNOS

#endif // BPMNOS_Token_H

