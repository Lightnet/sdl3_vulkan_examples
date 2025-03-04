#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cglm/cglm.h>

// Application state
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *text_texture;
    SDL_AppResult app_quit;
} AppContext;

// Create a texture with "Hello World" text using FreeType
SDL_Texture* create_text_texture(SDL_Renderer *renderer, FT_Library ft, const char *text, const char *font_path, int font_size) {
    FT_Face face;
    if (FT_New_Face(ft, font_path, 0, &face)) {
        SDL_Log("Failed to load font: %s", font_path);
        return NULL;
    }

    FT_Set_Pixel_Sizes(face, 0, font_size);

    // Calculate text dimensions with proper metrics
    const char *str = text;
    int width = 0, max_height = 0, max_descender = 0;
    for (const char *c = str; *c; c++) {
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER)) continue;
        width += (face->glyph->advance.x >> 6); // Advance in pixels (26.6 fixed-point to integer)
        int glyph_height = face->glyph->bitmap.rows;
        int glyph_top = face->glyph->bitmap_top;
        int descender = glyph_height - glyph_top; // Distance below baseline
        if (glyph_height > max_height) max_height = glyph_height;
        if (descender > max_descender) max_descender = descender;
    }
    int height = max_height + max_descender; // Total height including ascenders and descenders

    // Create surface with SDL_PIXELFORMAT_RGBA32
    SDL_Surface *surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        FT_Done_Face(face);
        return NULL;
    }

    // Get pixel format details for RGBA32
    const SDL_PixelFormatDetails *format_details = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);
    if (!format_details) {
        SDL_Log("Failed to get pixel format details: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        FT_Done_Face(face);
        return NULL;
    }

    // Render text to surface with proper glyph positioning
    int x_offset = 0;
    for (const char *c = str; *c; c++) {
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER)) continue;
        FT_Bitmap *bitmap = &face->glyph->bitmap;
        int y_offset = max_height - face->glyph->bitmap_top; // Align to baseline
        for (int y = 0; y < bitmap->rows; y++) {
            for (int x = 0; x < bitmap->width; x++) {
                Uint8 alpha = bitmap->buffer[y * bitmap->pitch + x];
                if (alpha) {
                    SDL_Rect pixel = {x_offset + face->glyph->bitmap_left + x, y_offset + y, 1, 1};
                    Uint32 color = SDL_MapRGBA(format_details, NULL, 255, 255, 255, alpha);
                    SDL_FillSurfaceRect(surface, &pixel, color);
                }
            }
        }
        x_offset += (face->glyph->advance.x >> 6); // Move to next glyph position
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    FT_Done_Face(face);
    return texture;
}

// SDL3 callback: Initialize the application
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("Hello SDL3 Text", 640, 480, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        return SDL_APP_FAILURE;
    }

    // Initialize FreeType
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        SDL_Log("FreeType initialization failed");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return SDL_APP_FAILURE;
    }

    // Replace with your font path
    const char *font_path = "C:/Windows/Fonts/arial.ttf";
    SDL_Texture *text_texture = create_text_texture(renderer, ft, "Hello World", font_path, 48);
    if (!text_texture) {
        FT_Done_FreeType(ft);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return SDL_APP_FAILURE;
    }

    // Set up application state
    AppContext *app = SDL_calloc(1, sizeof(AppContext));
    app->window = window;
    app->renderer = renderer;
    app->text_texture = text_texture;
    app->app_quit = SDL_APP_CONTINUE;
    *appstate = app;

    SDL_SetPointerProperty(SDL_GetRendererProperties(renderer), "freetype_library", ft);

    return SDL_APP_CONTINUE;
}

// SDL3 callback: Handle events
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppContext *app = (AppContext *)appstate;
    switch (event->type) {
        case SDL_EVENT_QUIT:
            app->app_quit = SDL_APP_SUCCESS;
            break;
    }
    return SDL_APP_CONTINUE;
}

// SDL3 callback: Render each frame
SDL_AppResult SDL_AppIterate(void *appstate) {
    AppContext *app = (AppContext *)appstate;

    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);

    float tex_w, tex_h;
    SDL_GetTextureSize(app->text_texture, &tex_w, &tex_h);
    int win_w, win_h;
    SDL_GetWindowSize(app->window, &win_w, &win_h); // Get dynamic window size
    SDL_FRect dst = {(win_w - tex_w) / 2.0f, (win_h - tex_h) / 2.0f, tex_w, tex_h};
    SDL_RenderTexture(app->renderer, app->text_texture, NULL, &dst);

    SDL_RenderPresent(app->renderer);
    return app->app_quit;
}

// SDL3 callback: Cleanup
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    AppContext *app = (AppContext *)appstate;
    if (app) {
        FT_Library ft = SDL_GetPointerProperty(SDL_GetRendererProperties(app->renderer), "freetype_library", NULL);
        if (ft) FT_Done_FreeType(ft);
        SDL_DestroyTexture(app->text_texture);
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        SDL_free(app);
    }
    SDL_Quit();
}