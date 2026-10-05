# Capitve Portal

A simple ESP12 captive portal to quickly integrate in prototypes.

## Prototype integration
Your sketch:

```
#include [your ..]
#include [includes ..]
#include "src/CaptivePortal.h"

[vars / defines]

void setup() {

  Serial.begin(115200);

  Serial.println("");
  Serial.println("Setup, begin");

  ESPPortal p;

  p.begin();
	
}

void loop() {

}
```

## Examples
See the examples dir

## Roadmap
- SSID input as dropdown with signal strength
- getters for wifi, web server & dns objs