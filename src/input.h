#ifndef INPUT_H
#define INPUT_H

int readKey();
void enableRawMode();
void disableRawMode();

// Arrow key definitions
#define ARROW_UP     1000
#define ARROW_DOWN   1001
#define ARROW_LEFT   1002
#define ARROW_RIGHT  1003

#endif // INPUT_H