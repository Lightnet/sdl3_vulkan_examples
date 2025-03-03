#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    printf("Starting SDL test...\n");
    fflush(stdout); // Ensure output is visible immediately


        //SDL_Init(SDL_INIT_VIDEO);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        fflush(stderr);
        return 1;
    }

    printf("SDL initialized successfully\n");
    SDL_Quit();
    return 0;
}