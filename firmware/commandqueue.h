#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include "command.h"

class CommandQueue {
public:
  static CommandQueue * Instance();
  Command * head, * tail;

  void enqueue( Command * c );
  void push( Command * c );
  Command * dequeue();

  uint8_t count;

private:
  static CommandQueue * instance;
  CommandQueue();
};

#endif
