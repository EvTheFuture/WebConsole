/*
 * Copyright (C) 2023 EvTheFuture <magnus.sandin@valitron.se>
 *
 * This file is part of the WebConsole.
 *
 * WebConsole is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * WebConsole is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WebConsole. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEBCONSOLE_H
#define WEBCONSOLE_H

#include <WebSocketsServer.h>

class WebConsole {

public:
  WebConsole(
          int port,
          int maxBufferLines,
          bool enableSerialOutput = false
          ) :
    webSocket(port),
    maxBufferLines(maxBufferLines),
    serialOutputEnabled(enableSerialOutput),
    lastCallHadNewLine(true) { }

  // Typedef for the callback function
  typedef String (*CommandCallback)(const String&);

  // Method to register the callback function
  void registerCommandCallback(CommandCallback callback);

  // Returns HTML to render the console page
  String getConsolePage() const;

  // Returns /script.js
  String getConsoleScript() const;

  // Get the current lines in the buffer
  String getBuffer() const;

  // Call this when WiFi has been connected
  void begin();

  // Set Epoch Time from external source
  void setEpoch(unsigned long epoch);

  // Call this inside your loop()
  void loop();

  template <class T> void print(const T &message) {
    logInternal(String(message), false);
  }

  template <class T> void println(const T &message) {
    logInternal(String(message), true);
  }

  // For formatted output
  template<typename... Args> void printf(const char* format, Args... args) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), format, args...);
    logInternal(buffer, false);

    int index = strlen(buffer) - 1;
    lastCallHadNewLine = (index >= 0 && buffer[index] == 0x0a);
  }

private:
  unsigned long lastMillis = 0;
  unsigned long epochDelta = 0;
  WebSocketsServer webSocket;
  CommandCallback commandCallback;
  bool serialOutputEnabled;
  String consoleBuffer;
  bool lastCallHadNewLine;
  const int maxBufferLines;

  void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
  void logInternal(const String& message, bool newline);
  String getFormattedTime();
  String pad(uint8_t in);
};

#endif
