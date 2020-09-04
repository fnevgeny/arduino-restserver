#ifndef restserver_h
#define restserver_h

// Include Arduino header
#include "Arduino.h"
#include <Ethernet.h>
#include "RestSettings.h"

struct Routes {
  const char * method;
  const char * name;
  void (*callback)(const char * params);
};

class RestServer {
public:
  RestServer(EthernetServer& client);
  
  void run();
  
  void addRoute(const char * method, const char * name, void (*f)(const char *));
  
  void addData(const char* name, uint16_t value);
  void addData(const char* name, int value);
  void addData(const char* name, float value);
  void addData(const char* name, const char* value);

private:
  Routes routes_[ROUTES_TOTAL];
  uint8_t routesIndex_;
  char buffer_[OUTPUT_BUFFER_SIZE];
  uint16_t bufferIndex_;
  
  EthernetServer& server_;
  EthernetClient client_;
  
  void check();
  void reset();
  void addToBuffer(const char * value);
  void send(uint8_t chunkSize, uint8_t delayTime);
};

#endif
