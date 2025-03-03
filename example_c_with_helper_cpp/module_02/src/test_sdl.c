#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    printf("Starting SDL test...\n");
    fflush(stdout); // Ensure output is visible immediately

    if (!SDL_Init(SDL_INIT_VIDEO)) {
      // This runs on FAILURE (non-zero return value)
      fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
      SDL_Quit();
      return 1; // Return 1 to indicate failure
    } else {
        // This runs on SUCCESS (zero return value)
        printf("SDL initialized successfully\n");
        // No SDL_Quit() here yet; continue with the program
        // return 0; // Don’t return yet in vsdl_init_core
    }

    printf("SDL initialized successfully\n");
    SDL_Quit();
    return 0;
}