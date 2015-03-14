#include "commandqueue.h"
#include "const.h"

CommandQueue * CommandQueue::instance = NULL;

CommandQueue * CommandQueue::Instance() {
	if( instance == NULL )
		instance = new CommandQueue();
	return instance;
}

CommandQueue::CommandQueue() {
	tail = head = NULL;
	count = 0;
}

// Add new command to end of queue
void CommandQueue::enqueue( Command * c ) {
	count++;
	if( tail == NULL ) {
		head = tail = c;
	} else {
		tail->next = c;
		tail = c;
	}
#ifdef DEBUG_QUEUE
	Command * cptr = head;
	while( cptr != NULL ) {
		Serial.print( " Q: " );
		print_command( Serial, cptr );
		cptr = cptr->next;
	}
	Serial.flush();
#endif
}

// We can use this if a command wants to represent itself
// as multiple commands
void CommandQueue::push( Command * c ) {
	count++;
	if( head == NULL ) {
		head = tail = c;
	} else {
		c->next = head;
		head = c;
	}
#ifdef DEBUG_QUEUE
	Command * cptr = head;
	while( cptr != NULL ) {
		Serial.print( " Q: " );
		print_command( Serial, cptr );
		cptr = cptr->next;
	}
	Serial.flush();
#endif
}

// Remove first command from start of queue
Command * CommandQueue::dequeue() {
	if( count == 0 ) {
		return NULL;
	}
	count--;
	Command * p = head;
	if( head->next == NULL ) {
		head = tail = NULL;
	} else {
		head = head->next;
#ifdef DEBUG_QUEUE
	Command * cptr = head;
	while( cptr != NULL ) {
		Serial.print( " Q: " );
		print_command( Serial, cptr );
		cptr = cptr->next;
	}
#endif
	}
	return p;
}

