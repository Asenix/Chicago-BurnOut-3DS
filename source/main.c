
#include <3ds.h>
#include <stdio.h>

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("CHICAGO BURNOUT\n");
    printf("------------------------------\n");
    printf("Open-city prototype\n\n");
    printf("Prototype 0.1\n\n");
    printf("3D city coming next!\n\n");
    printf("Press START to exit.\n");

    while (aptMainLoop()) {
        hidScanInput();

        if (hidKeysDown() & KEY_START)
            break;

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
