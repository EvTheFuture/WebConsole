# WebConsole

WebConsole is an Arduino library that provides a simple console window on a web page that allow to view log output in realtime and also to send commands which will be sent to a callback funtion (if registered). It can optionally also print to Serial if desired.

[![buy-me-a-coffee](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/EvTheFuture)

## Features
- Simultaneously log to Web Console and Serial or only log to Web Console
- Read logging on a web page
- Send commands from the web page
- Specify a Command Callback function to handle commands

## Usage

Simply include the library and create an instance of the console

```cpp
#include <WebConsole.h>

// <Web Socket Port>, <Buffer size>, <Also print to Serial>
WebConsole console(81, 50, true);
```

## Examples
The following example is provided to demonstrate the usage.
- `WebConsoleExample.ino`

## License
This library is licensed under the GNU General Public License v3.0. See the LICENSE file for details.
