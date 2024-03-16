#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <ctime>

/// Window settings
const int SCREEN_WIDTH = 1080;
const int SCREEN_HEIGHT = 720;

/// Paddle settings
const int PADDLE_WIDTH = 10;
const int PADDLE_HEIGHT = 100;
int playerPaddlePosY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2; ///< Y start position of the player's paddle
int aiPaddlePosY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;     ///< Y start position of the AI's paddle
const int PADDLE_SPEED = 15;
const int AI_PADDLE_SPEED = 4; ///< AI paddle speed

/// Ball settings
const int BALL_SIZE = 10;
int ballPosX = SCREEN_WIDTH / 2 - BALL_SIZE / 2; ///< X start position of the ball
int ballPosY = SCREEN_HEIGHT / 2 - BALL_SIZE / 2; ///< Y start position of the ball
int ballVelX = -6; ///< Ball X velocity
int ballVelY = -6; ///< Ball Y velocity

/// Scores
int playerScore = 0;
int aiScore = 0;
Mix_Chunk* soundPoint = nullptr;
Mix_Chunk* soundHit = nullptr;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

/**
 * Initializes SDL, creates window and renderer, and loads sounds.
 * @return True if initialization is successful, false otherwise.
 */
bool init() {
    srand(static_cast<unsigned int>(time(nullptr)));
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return false;
    }

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("SDL Pong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    soundPoint = Mix_LoadWAV("point.wav");
    if (soundPoint == nullptr) {
        std::cerr << "Failed to load sound effect! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    soundHit = Mix_LoadWAV("hit.wav");
    if (soundHit == nullptr) {
        std::cerr << "Failed to load sound effect! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    return true;
}

/**
 * Cleans up and quits SDL.
 */
void close() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_FreeChunk(soundPoint);
    soundPoint = nullptr;
    Mix_FreeChunk(soundHit);
    soundHit = nullptr;
    Mix_CloseAudio();
    window = nullptr;
    renderer = nullptr;
    TTF_Quit();
    SDL_Quit();
}

/**
 * Handles input events from the user, such as keyboard presses.
 * @param quit Reference to the quit flag to signal game exit.
 * @param e SDL_Event structure to handle events.
 */
void handleInput(bool& quit, SDL_Event& e) {
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                    playerPaddlePosY -= PADDLE_SPEED;
                    if (playerPaddlePosY < 0) {
                        playerPaddlePosY = 0;
                    }
                    break;
                case SDLK_DOWN:
                    playerPaddlePosY += PADDLE_SPEED;
                    if (playerPaddlePosY > SCREEN_HEIGHT - PADDLE_HEIGHT) {
                        playerPaddlePosY = SCREEN_HEIGHT - PADDLE_HEIGHT;
                    }
                    break;
            }
        }
    }
}

/**
 * Moves the ball by updating its position based on its velocity. Also handles
 * ball collisions with top/bottom edges to bounce, scoring when ball goes off
 * screen, and collisions with player/AI paddles. Plays sound effects for bounces
 * and scoring.
 */
void moveBall() {
    ballPosX += ballVelX;
    ballPosY += ballVelY;

    // Bouncing off the top and bottom edges
    if (ballPosY < 0 || ballPosY + BALL_SIZE > SCREEN_HEIGHT) {
        ballVelY = -ballVelY;
        Mix_PlayChannel(-1, soundHit, 0);
    }

    // Reset ball and scoring
    if (ballPosX < 0) {
        Mix_PlayChannel(-1, soundPoint, 0);
        aiScore++;
        ballPosX = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
        ballPosY = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;
        ballVelX = 6; // Change ball movement direction
    } else if (ballPosX + BALL_SIZE > SCREEN_WIDTH) {
        Mix_PlayChannel(-1, soundPoint, 0);
        playerScore++;
        ballPosX = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
        ballPosY = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;
        ballVelX = -6; // Change ball movement direction
    }

    // Improved collision detection with player paddle
    if (ballPosX < PADDLE_WIDTH &&
        ballPosY >= playerPaddlePosY &&
        ballPosY <= playerPaddlePosY + PADDLE_HEIGHT) {
        ballVelX = -ballVelX;
        Mix_PlayChannel(-1, soundHit, 0);
    }

    // Improved collision detection with AI paddle
    if (ballPosX + BALL_SIZE > SCREEN_WIDTH - PADDLE_WIDTH &&
        ballPosY >= aiPaddlePosY &&
        ballPosY <= aiPaddlePosY + PADDLE_HEIGHT) {
        ballVelX = -ballVelX;
        Mix_PlayChannel(-1, soundHit, 0);
    }
}

/**
 * Updates the AI paddle's position based on the ball position.
 * Moves the paddle towards the ball, with some randomization.
 * Clamps the paddle position to remain on screen.
 */
void updateAI() {
    // Check ball position
    if (ballPosY > aiPaddlePosY + PADDLE_HEIGHT / 2) {
        // Ball is below the center of AI paddle, move paddle down
        aiPaddlePosY += rand() % 8 + 1;
        if (aiPaddlePosY > SCREEN_HEIGHT - PADDLE_HEIGHT) {
            aiPaddlePosY = SCREEN_HEIGHT - PADDLE_HEIGHT;
        }
    } else if (ballPosY < aiPaddlePosY + PADDLE_HEIGHT / 2) {
        // Ball is above the center of AI paddle, move paddle up
        aiPaddlePosY -= rand() % 8 + 1;
        if (aiPaddlePosY < 0) {
            aiPaddlePosY = 0;
        }
    }
}

/**
 * Renders the game state to the screen, including paddles, ball, and score.
 */
void render() {
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer);

    // Draw player paddle
    SDL_Rect playerPaddle = {PADDLE_WIDTH, playerPaddlePosY, PADDLE_WIDTH, PADDLE_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(renderer, &playerPaddle);

    // Draw AI paddle
    SDL_Rect aiPaddle = {SCREEN_WIDTH - (PADDLE_WIDTH*2), aiPaddlePosY, PADDLE_WIDTH, PADDLE_HEIGHT};
    SDL_RenderFillRect(renderer, &aiPaddle);

    // Draw ball
    SDL_Rect ball = {ballPosX, ballPosY, BALL_SIZE, BALL_SIZE};
    SDL_RenderFillRect(renderer, &ball);

    // Rendering score
    TTF_Font* font = TTF_OpenFont("font.ttf", 100); // Change path and size accordingly
    if (font == nullptr) {
        std::cerr << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << std::endl;
    } else {
        SDL_Color textColor = {255, 0, 0, 255};
        std::string scoreText = "You: " + std::to_string(playerScore) + " AI: " + std::to_string(aiScore);
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreText.c_str(), textColor);
        if (textSurface == nullptr) {
            std::cerr << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        } else {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture == nullptr) {
                std::cerr << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
            } else {
                // Render the text
                SDL_Rect renderQuad = {(SCREEN_WIDTH - textSurface->w) / 2, 20, textSurface->w, textSurface->h};
                SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
        TTF_CloseFont(font);
    }

    SDL_RenderPresent(renderer);
}

/**
 * The main game loop. Initializes the game, then enters a loop handling input,
 * moving the ball, updating AI, and rendering the game state until the game is quit.
 * @return Exit code.
 */
int main(int argc, char* argv[]) {
    if (!init()) {
        std::cerr << "Failed to initialize!" << std::endl;
        return -1;
    }
    bool quit = false;
    SDL_Event e;

    Uint32 startTick, frameTime;
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;

    while (!quit) {
        startTick = SDL_GetTicks();
        handleInput(quit, e);
        moveBall();
        updateAI(); // Update AI paddle position
        render();

        frameTime = SDL_GetTicks() - startTick;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    std::cout << "Final Score - Player: " << playerScore << " AI: " << aiScore << std::endl;

    close();
    return 0;
}
