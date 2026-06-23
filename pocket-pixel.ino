#include "src/PocketPixelGame.h"

void setup() {
    gameSetup();
}

void loop() {
    gameLoop();
}

#ifdef POCKET_PIXEL_FXC_LINK
int main(void) {
    init();
    setup();
    for (;;) {
        loop();
    }
}
#endif
