#include <SDL.h>       // core nya buat gui juga
#include <SDL_mixer.h> // buat audio processing
#include <SDL_ttf.h>   // buat text processing
#include <iostream>    // standard lib
#include <string>      // buat string
#include <iomanip>
#include <sstream>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h> // cuma bisa buat windows TwT
#include <commdlg.h>
#include <shlobj.h>
#endif
#define MAX_PATH 260

const int WINDOW_WIDTH = 700;        // lebar layar
const int WINDOW_HEIGHT = 500;       // tinggi layar
const int BUTTON_WIDTH = 200;        // lebar tombol
const int BUTTON_HEIGHT = 50;        // tinggi tombol
const int PROGRESS_BAR_HEIGHT = 20;  // tinggi progress bar
const int VOLUME_SLIDER_WIDTH = 200; // lebar slider volume
const int VOLUME_SLIDER_HEIGHT = 20; // tinggi slider volume

// UI States
struct Button
{
    SDL_Rect rect;
    std::string text;
    bool hovered;
    SDL_Color textColor;
    SDL_Color bgColor;
    SDL_Color hoverColor;
};

struct Slider
{
    SDL_Rect rect;
    float value; // 0.0 to 1.0
    bool dragging;
};

struct PlayerState
{
    Mix_Music *music;
    bool isPlaying;
    bool musicLoaded;
    std::string currentSong;
    double songLength; // in seconds my niga
    Slider volumeSlider;
    Slider progressBar;
};

std::string formatTime(int seconds)
{
    int minutes = seconds / 60;
    seconds = seconds % 60;
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << seconds;
    return ss.str();
}

std::string openFileDialog()
{
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "MP3 Files\0*.mp3\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "mp3";

    std::string fileNameStr;
    if (GetOpenFileNameA(&ofn))
    {
        fileNameStr = fileName;
    }
    return fileNameStr;
}

void renderText(SDL_Renderer *renderer, TTF_Font *font, const std::string &text,
                int x, int y, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (surface == nullptr)
    {
        std::cout << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr)
    {
        std::cout << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect renderQuad = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &renderQuad);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void renderButton(SDL_Renderer *renderer, TTF_Font *font, const Button &button)
{
    // Draw button background
    SDL_SetRenderDrawColor(renderer,
                           button.hovered ? button.hoverColor.r : button.bgColor.r,
                           button.hovered ? button.hoverColor.g : button.bgColor.g,
                           button.hovered ? button.hoverColor.b : button.bgColor.b,
                           255);
    SDL_RenderFillRect(renderer, &button.rect);

    // Draw button border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &button.rect);

    // Center the text in the button
    int textX = button.rect.x + (button.rect.w / 2);
    int textY = button.rect.y + (button.rect.h / 2);
    renderText(renderer, font, button.text,
               textX - (button.text.length() * 8), // Approximate center
               textY - 12,                         // Approximate center
               button.textColor);
}

void renderSlider(SDL_Renderer *renderer, const Slider &slider, bool isProgress)
{
    // Draw background
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &slider.rect);

    // Draw filled portion
    SDL_Rect fillRect = slider.rect;
    fillRect.w = static_cast<int>(slider.rect.w * slider.value);
    SDL_SetRenderDrawColor(renderer, isProgress ? 70 : 46,
                           isProgress ? 130 : 139,
                           isProgress ? 180 : 87, 255);
    SDL_RenderFillRect(renderer, &fillRect);

    // Draw border
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &slider.rect);
}

bool handleSliderClick(Slider &slider, int mouseX, int mouseY)
{
    if (mouseX >= slider.rect.x && mouseX <= slider.rect.x + slider.rect.w &&
        mouseY >= slider.rect.y && mouseY <= slider.rect.y + slider.rect.h)
    {
        float newValue = static_cast<float>(mouseX - slider.rect.x) / slider.rect.w;
        newValue = std::max(0.0f, std::min(1.0f, newValue));
        slider.value = newValue;
        return true;
    }
    return false;
}

#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1)
    {
        std::cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        std::cout << "SDL_mixer could not initialize! Mix_Error: " << Mix_GetError() << std::endl;
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow(
        "Stelle MP3 Loader",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (window == nullptr)
    {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Load and set the window icon
    SDL_Surface *icon = SDL_LoadBMP("../icon.ico");
    if (icon != nullptr)
    {
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load font
    TTF_Font *font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 24);
    if (font == nullptr)
    {
        std::cout << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    // Create buttons
    Button loadButton = {
        {WINDOW_WIDTH / 2 - BUTTON_WIDTH - 20, WINDOW_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT},
        "Load MP3",
        false,
        {255, 255, 255, 255}, // Text color (white)
        {70, 130, 180, 255},  // Background color (steel blue)
        {100, 149, 237, 255}  // Hover color (cornflower blue)
    };

    Button playButton = {
        {WINDOW_WIDTH / 2 + 20, WINDOW_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT},
        "Play",
        false,
        {255, 255, 255, 255}, // Text color (white)
        {46, 139, 87, 255},   // Background color (sea green)
        {60, 179, 113, 255}   // Hover color (medium sea green)
    };

    // Initialize player state
    PlayerState playerState = {
        nullptr, false, false, "No song loaded", 0.0,
        {// Volume slider
         {WINDOW_WIDTH - VOLUME_SLIDER_WIDTH - 20, 20, VOLUME_SLIDER_WIDTH, VOLUME_SLIDER_HEIGHT},
         0.5f, // Initial volume at 50%
         false},
        {// Progress bar
         {20, WINDOW_HEIGHT - PROGRESS_BAR_HEIGHT - 20, WINDOW_WIDTH - 40, PROGRESS_BAR_HEIGHT},
         0.0f,
         false}};

    // Set initial volume
    Mix_VolumeMusic(static_cast<int>(playerState.volumeSlider.value * MIX_MAX_VOLUME));

    bool quit = false;
    bool needsRedraw = true; // Flag to track if we need to redraw the UI
    SDL_Event e;

    // Frame timing
    const int FPS = 30;
    const int frameDelay = 1000 / FPS;
    Uint32 frameStart;
    int frameTime;

    while (!quit)
    {
        frameStart = SDL_GetTicks();

        // Update progress bar if music is playing
        if (playerState.isPlaying && playerState.musicLoaded)
        {
            if (Mix_PlayingMusic())
            {
                playerState.progressBar.value = static_cast<float>(Mix_GetMusicPosition(playerState.music));
                needsRedraw = true;
            }
        }

        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            else if (e.type == SDL_MOUSEMOTION)
            {
                int mouseX = e.motion.x;
                int mouseY = e.motion.y;

                // Check button hover states and set needsRedraw only if hover state changes
                bool prevLoadHovered = loadButton.hovered;
                bool prevPlayHovered = playButton.hovered;

                loadButton.hovered = (mouseX >= loadButton.rect.x &&
                                      mouseX <= loadButton.rect.x + loadButton.rect.w &&
                                      mouseY >= loadButton.rect.y &&
                                      mouseY <= loadButton.rect.y + loadButton.rect.h);

                playButton.hovered = (mouseX >= playButton.rect.x &&
                                      mouseX <= playButton.rect.x + playButton.rect.w &&
                                      mouseY >= playButton.rect.y &&
                                      mouseY <= playButton.rect.y + playButton.rect.h);

                if (prevLoadHovered != loadButton.hovered || prevPlayHovered != playButton.hovered)
                {
                    needsRedraw = true;
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                int mouseX = e.button.x;
                int mouseY = e.button.y;

                // Handle volume slider click
                if (handleSliderClick(playerState.volumeSlider, mouseX, mouseY))
                {
                    Mix_VolumeMusic(static_cast<int>(playerState.volumeSlider.value * MIX_MAX_VOLUME));
                    needsRedraw = true;
                }

                // Handle progress bar click
                if (playerState.musicLoaded && handleSliderClick(playerState.progressBar, mouseX, mouseY))
                {
                    double newPosition = playerState.progressBar.value * playerState.songLength;
                    Mix_SetMusicPosition(newPosition);
                    needsRedraw = true;
                }

                // Load button clicked
                if (mouseX >= loadButton.rect.x &&
                    mouseX <= loadButton.rect.x + loadButton.rect.w &&
                    mouseY >= loadButton.rect.y &&
                    mouseY <= loadButton.rect.y + loadButton.rect.h)
                {

                    std::string filePath = openFileDialog();
                    if (!filePath.empty())
                    {
                        if (playerState.music != nullptr)
                        {
                            Mix_FreeMusic(playerState.music);
                            playerState.music = nullptr;
                        }

                        playerState.music = Mix_LoadMUS(filePath.c_str());
                        if (playerState.music == nullptr)
                        {
                            std::cout << "Failed to load music! SDL_mixer Error: " << Mix_GetError() << std::endl;
                            playerState.currentSong = "Failed to load song";
                            playerState.musicLoaded = false;
                        }
                        else
                        {
                            playerState.musicLoaded = true;
                            size_t lastSlash = filePath.find_last_of("/\\");
                            playerState.currentSong = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
                            // We'll update the progress bar based on music position
                            playerState.songLength = 0;
                            playerState.progressBar.value = 0;
                        }
                        needsRedraw = true;
                    }
                }

                // Play/Pause button clicked
                if (mouseX >= playButton.rect.x &&
                    mouseX <= playButton.rect.x + playButton.rect.w &&
                    mouseY >= playButton.rect.y &&
                    mouseY <= playButton.rect.y + playButton.rect.h)
                {

                    if (playerState.musicLoaded)
                    {
                        if (playerState.isPlaying)
                        {
                            Mix_PauseMusic();
                            playerState.isPlaying = false;
                            playButton.text = "Play";
                        }
                        else
                        {
                            Mix_PlayMusic(playerState.music, -1);
                            playerState.isPlaying = true;
                            playButton.text = "Pause";
                        }
                        needsRedraw = true;
                    }
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP)
            {
                playerState.volumeSlider.dragging = false;
                playerState.progressBar.dragging = false;
            }
            else if (e.type == SDL_MOUSEMOTION && (playerState.volumeSlider.dragging || playerState.progressBar.dragging))
            {
                int mouseX = e.motion.x;
                if (playerState.volumeSlider.dragging)
                {
                    handleSliderClick(playerState.volumeSlider, mouseX, e.motion.y);
                    Mix_VolumeMusic(static_cast<int>(playerState.volumeSlider.value * MIX_MAX_VOLUME));
                    needsRedraw = true;
                }
                if (playerState.progressBar.dragging && playerState.musicLoaded)
                {
                    handleSliderClick(playerState.progressBar, mouseX, e.motion.y);
                    double newPosition = playerState.progressBar.value * playerState.songLength;
                    Mix_SetMusicPosition(newPosition);
                    needsRedraw = true;
                }
            }
        }

        if (needsRedraw)
        {
            // Clear screen
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255); // Dark background
            SDL_RenderClear(renderer);

            // Draw title
            SDL_Color titleColor = {255, 255, 255, 255};
            renderText(renderer, font, "Stelle MP3 Loader",
                       WINDOW_WIDTH / 2 - 100, 50, titleColor);

            // Render current song info
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_Rect songInfoRect = {
                WINDOW_WIDTH / 2 - 250,
                WINDOW_HEIGHT / 2 - 100,
                500,
                40};
            SDL_RenderFillRect(renderer, &songInfoRect);
            renderText(renderer, font, "Current Song: " + playerState.currentSong,
                       WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT / 2 - 90, {200, 200, 200, 255});

            // Render volume slider
            renderSlider(renderer, playerState.volumeSlider, false);
            renderText(renderer, font, "Volume",
                       WINDOW_WIDTH - VOLUME_SLIDER_WIDTH - 20,
                       5, {200, 200, 200, 255});

            // Render progress bar and time
            renderSlider(renderer, playerState.progressBar, true);
            if (playerState.musicLoaded)
            {
                int currentTime = static_cast<int>(playerState.progressBar.value);
                int totalTime = static_cast<int>(playerState.songLength);
                std::string timeText = formatTime(currentTime);
                renderText(renderer, font, timeText,
                           WINDOW_WIDTH / 2 - 50,
                           WINDOW_HEIGHT - PROGRESS_BAR_HEIGHT - 45,
                           {200, 200, 200, 255});
            }

            // Render buttons
            renderButton(renderer, font, loadButton);
            renderButton(renderer, font, playButton);

            SDL_RenderPresent(renderer);
            needsRedraw = false;
        }

        // Cap the frame rate
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime)
        {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    // Cleanup
    if (playerState.music != nullptr)
    {
        Mix_FreeMusic(playerState.music);
    }
    TTF_CloseFont(font);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
