#include "RestServer.h"

RestServer::RestServer(EthernetServer& server): server_(server), routesIndex_(0), bufferIndex_(0) {
}

void RestServer::run() {
  client_ = server_.available();
  if (client_) {
    JSON_START();
    // Check the received request and process it
    check();
    if (bufferIndex_ > 0) {
      bufferIndex_--;
    }
    JSON_CLOSE();

    // Send data for the client
    send(8, 0);

    // Stop the client connection
    client_.stop();

    // Necessary resets
    reset();
  }
}

void RestServer::reset() {
  // Reset buffer
  memset(buffer_, 0, sizeof(buffer_));

  // Reset buffer index
  bufferIndex_ = 0;
}

void RestServer::addRoute(const char * method, const char * name,
    void (*f)(const char * params) ) {
  if (routesIndex_ >= ROUTES_TOTAL) {
    return;
  }
  routes_[routesIndex_].method   = method;
  routes_[routesIndex_].name     = name;
  routes_[routesIndex_].callback = f;

  routesIndex_++;
}

void RestServer::addToBuffer(const char * value) {
  if (strlen(value) + bufferIndex_ >= OUTPUT_BUFFER_SIZE) {
    return;
  }
  strcpy(buffer_ + bufferIndex_, value);
  bufferIndex_ = bufferIndex_ + strlen(value);
}

// Add to output buffer_
void RestServer::addData(const char* name, const char * value) {
  // Format the data as:
  // "name":"value",
  addToBuffer("\"");
  addToBuffer(name);
  addToBuffer("\":\"");
  addToBuffer(value);
  addToBuffer("\",");
}

void RestServer::addData(const char* name, uint16_t value){
  char number[10];
  itoa(value, number, 10);
  
  addData(name, number);
}

void RestServer::addData(const char* name, int value){
  char number[10];
  itoa(value, number, 10);
  
  addData(name, number);
}

void RestServer::addData(const char* name, float value){
  char number[10];
  dtostrf(value, 5, 2, number);
  
  addData(name, number);
}

// Send the HTTP response for the client
void RestServer::send(uint8_t chunkSize, uint8_t delayTime) {
  // First, send the HTTP Common Header
  client_.println(HTTP_COMMON_HEADER);

  // Send all of it
  if (chunkSize == 0) {
    client_.print(buffer_);
    return;
  }

  // Number of full chunks
  uint8_t nchunks = bufferIndex_/chunkSize;

  // Send chunk by chunk
  for (uint8_t i = 0; i < nchunks; i++) {
    client_.write(buffer_ + i*chunkSize, chunkSize);

    // Wait for client_ to get data
    delay(delayTime);
  }
  // Anything that has been left..
  if (nchunks*chunkSize < bufferIndex_) {
    client_.print(buffer_ + nchunks*chunkSize);
  }
}

// Extract information about the HTTP Header
void RestServer::check() {
  char route[ROUTES_LENGHT] = {0};
  bool routePrepare = false;
  bool routeCatchFinished = false;
  uint8_t r = 0;

  char query[QUERY_LENGTH] = {0};
  bool queryPrepare = false;
  bool queryCatchFinished = false;
  uint8_t q = 0;
  
  char method[METHODS_LENGTH] = {0};
  bool methodCatchFinished = false;
  uint8_t m = 0;

  bool currentLineIsBlank = true;
  char c;
  while ( client_.connected() && client_.available() ) {
    c = client_.read();

    // Start end of line process ////////////////
    // if you've gotten to the end of the line (received a newline
    // character) and the line is blank, the http request header has ended,
    // so you can send a reply or check the body of the http header
    if (c == '\n' && currentLineIsBlank) {
      // Here is where the parameters of other HTTP Methods will be.
      while(client_.available() && client_.connected())
        query[q++] = (client_.read());
      
      break;
    }

    if (c == '\n')
      currentLineIsBlank = true; // you're starting a new line
    else if (c != '\r')
      currentLineIsBlank = false; // you've gotten a character on the current line
    // End end of line process //////////////////

    // Start route catch process ////////////////
    if(c == '/' && !routePrepare)
      routePrepare = true;

    if((c == ' ' || c == '?') && routePrepare)
      routeCatchFinished = true;

    if(routePrepare && !routeCatchFinished)
      route[r++] = c;
    // End route catch process //////////////////

    // Start query catch process ////////////////
    if(c == ' ' && queryPrepare)
      queryCatchFinished = true;

    if(queryPrepare && !queryCatchFinished)
      query[q++] = c;

    if(c == '?' && !queryPrepare)
      queryPrepare = true;
    // End query catch process /////////////////

    // Start method catch process ///////////////
    if(c == ' ' && !methodCatchFinished)
      methodCatchFinished = true;

    if(!methodCatchFinished)
      method[m++] = c;
    // End method catch process /////////////////

  }

  for(int i = 0; i < routesIndex_; i++) {
      // Check if the routes names matches
      if (strcmp(route, routes_[i].name) != 0)
        continue;

      // Check if the HTTP METHOD matters for this route
      if (strcmp(routes_[i].method, "*") != 0) {
        // If it matters, check if the methods matches
        if (strcmp(method, routes_[i].method) != 0)
          continue;
      }

      // Route callback (function)
      routes_[i].callback(query);
  }
}
