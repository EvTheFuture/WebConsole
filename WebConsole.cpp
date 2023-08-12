/*
 * Copyright (C) 2023 EvTheFuture <magnus.sandin@valitron.se>
 *
 * This file is part of the ConsoleLibrary.
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

#include "WebConsole.h"

void WebConsole::begin() {
  // Register a lambda that captures 'this' and calls the member function
  webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    this->handleWebSocketEvent(num, type, payload, length);
  });

  // Start up the WebSocketsServer
  webSocket.begin();
}

void WebConsole::setEpoch(unsigned long epoch) {
    epochDelta = epoch - millis() / 1000;
}

void WebConsole::loop() {
  webSocket.loop();

  // Adjust deta if millis() wrap around (after 49 days)
  if (lastMillis > millis())
      epochDelta += 4294967;

  lastMillis = millis();
}

String WebConsole::getBuffer() const {
  return consoleBuffer;
}

String WebConsole::getFormattedTime()
{
  unsigned long epoch = millis() / 1000 + epochDelta;
  int ms = millis() % 1000;

  int days = (((epoch / 86400L) + 4 ) % 7); //0 is Sunday
  int hours = ((epoch % 86400L) / 3600);
  int minutes = ((epoch % 3600) / 60);
  int seconds = (epoch % 60);

  String s = pad(hours);
  s += ":" + pad(minutes);
  s += ":" + pad(seconds);
  s += "." + (ms < 10 ? "00" + String(ms) : (ms < 100 ? "0" + String(ms) : String(ms)));

  return s;
}

String WebConsole::pad(uint8_t in)
{
  return in < 10 ? "0" + String(in) : String(in);
}

void WebConsole::logInternal(const String &message, bool newline) {
  String timestampedMessage;

  if (lastCallHadNewLine) {
    // Add timestamp to the message
    String timestamp = "[" + getFormattedTime() + "] ";
    timestampedMessage = timestamp + message;
  } else {
    timestampedMessage = message;
  }

  if (serialOutputEnabled) {
      // Send the message to Serial
      Serial.print(message);
      if (newline) {
        Serial.println();
      }
  }

  // Update the newline state
  lastCallHadNewLine = newline;

  // Add the message to the console buffer
  consoleBuffer += timestampedMessage;
  if (newline) {
    consoleBuffer += "\n";
  }

  // Count the number of lines in the console buffer
  int numLines = 0;
  for (char c : consoleBuffer) {
    if (c == '\n') {
      numLines++;
    }
  }

  // Truncate the buffer if it exceeds the maximum number of lines
  while (numLines > maxBufferLines) {
    int newlineIndex = consoleBuffer.indexOf('\n');
    consoleBuffer.remove(0, newlineIndex + 1);
    numLines--;
  }

  // Prepare the message for broadcast
  String broadcastMessage = timestampedMessage;
  if (newline) {
    broadcastMessage += "\n";
  }

  // Send the message to all connected clients
  webSocket.broadcastTXT(broadcastMessage.c_str());
}

void WebConsole::handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t * pl, size_t length) {
  String payload = String(pl, length);

  switch (type) {
    case WStype_DISCONNECTED:
      this->printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        this->printf("[%u] Connection from %d.%d.%d.%d, URL: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;

    case WStype_TEXT:
      {
        this->printf("[%u] Received Command: '%s'\n", num, payload);
        if (commandCallback != nullptr) {
          String response = commandCallback(payload);
          String timestamp = "[" + getFormattedTime() + "] ";

          if (response != nullptr && response.length() > 0)
            webSocket.sendTXT(num, timestamp + response.c_str());
          else
            webSocket.sendTXT(num, timestamp + "DONE");
        }
      }
      break;

    case WStype_PONG:
    case WStype_PING:
      break;

    default:
      this->printf("[%u] Unknown event type: %d\n", num, type);
      break;
  }
}

void WebConsole::registerCommandCallback(CommandCallback callback) {
  commandCallback = callback;
}

String WebConsole::getConsolePage() const {
  return "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<script src='/script.js'></script>\n"
        "</head>\n"
        "<body style='background: #404040; color: #fff;' onLoad='document.getElementById(\"command-input\").addEventListener(\"keydown\", handleKeyPress);'>\n"
        "<textarea id='debug-output' style='width:100%; height:90vh; background: #202020; color: #fff;'>" + getBuffer() + "</textarea><br>\n"
        "<input type='text' id='command-input'><button onclick='sendCommand()'>Send</button>\n"
        "</body>\n"
        "</head>";
}

String WebConsole::getConsoleScript() const {
return "var socket;\n"
       "var commandHistory = [];\n"
       "var commandIndex = -1;\n"
       "var currentTypedCommand = \"\";\n"
       "var historySize = 1000;\n\n"
       "function connectWebSocket() {\n"
       "  socket = new WebSocket('ws://' + window.location.hostname + ':81/');\n"
       "  socket.onmessage = function(event) {\n"
       "    var message = event.data;\n"
       "    document.getElementById('debug-output').value += message;\n"
       "    document.getElementById('debug-output').scrollTop = document.getElementById('debug-output').scrollHeight;\n"
       "  };\n\n"
       "  socket.onopen = function() {\n"
       "    console.log('WebSocket connected');\n"
       "  };\n\n"
       "  socket.onclose = function() {\n"
       "    console.log('WebSocket closed, reconnecting...');\n"
       "    setTimeout(connectWebSocket, 1000); // Try to reconnect after 1 second\n"
       "  };\n"
       "}\n\n"
       "function handleKeyPress(event) {\n"
       "  var commandInput = document.getElementById('command-input');\n"
       "  if (event.key === 'Enter') {\n"
       "    sendCommand();\n"
       "  } else if (event.key === 'ArrowUp') {\n"
       "    if (commandIndex === -1) { currentTypedCommand = commandInput.value; }\n"
       "    if (commandIndex < commandHistory.length - 1) { commandIndex++; commandInput.value = commandHistory[commandIndex]; }\n"
       "  } else if (event.key === 'ArrowDown') {\n"
       "    if (commandIndex > -1) { commandIndex--; commandInput.value = commandIndex === -1 ? currentTypedCommand : commandHistory[commandIndex]; }\n"
       "  }\n"
       "}\n\n"
       "function sendCommand() {\n"
       "  var command = document.getElementById('command-input').value;\n"
       "  socket.send(command);\n"
       "  document.getElementById('command-input').value = '';\n"
       "  commandHistory.unshift(command);\n"
       "  if (commandHistory.length > historySize) { commandHistory.pop(); }\n"
       "  commandIndex = -1;\n"
       "  currentTypedCommand = \"\";\n"
       "}\n\n"
       "connectWebSocket();\n";
}

