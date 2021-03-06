/* ========================================================================== */
/*                          STC - SIMPLE TETRIS CLONE                         */
/* -------------------------------------------------------------------------- */
/*   Simple pure SDL implementation. (No sound, no fonts)                     */
/*   Using SDL for game state rendering, user input and timing.               */
/*                                                                            */
/*   Copyright (c) 2011 Laurens Rodriguez Oscanoa.                            */
/*   This code is licensed under the MIT license:                             */
/*   http://www.opensource.org/licenses/mit-license.php                       */
/* -------------------------------------------------------------------------- */

#include "sdl_game.hpp"
#include <cstdlib>
#include <ctime>
#include <SDL_image.h>

namespace stc
{

// Initializes platform, if there are no problems returns ERROR_NONE.
int PlatformSdl::init(Game *game)
{
    // Initialize the random number generator
    srand((unsigned int)(time(NULL)));

    // Start video and audio system
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        return Game::ERROR_PLATFORM;
    }

    // Create game video surface
    mScreen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT,
                              SCREEN_BIT_DEPTH,
                              SCREEN_VIDEO_MODE);
    if (mScreen == NULL)
    {
        return Game::ERROR_NO_VIDEO;
    }

    // Set window caption
    SDL_WM_SetCaption(STC_GAME_NAME " (C++)", STC_GAME_NAME);

    // Load images for blocks and background
    mBmpTiles = IMG_Load(STC_BMP_TILE_BLOCKS);
    mBmpBack = IMG_Load(STC_BMP_BACKGROUND);
    mBmpNumbers = IMG_Load(STC_BMP_NUMBERS);

    if (mBmpTiles == NULL || mBmpBack == NULL || mBmpNumbers == NULL)
    {
        return Game::ERROR_NO_IMAGES;
    }

	// Setup audio
	int audioRate = 44100;
	Uint16 audioFormat = AUDIO_S16; // 16-bit stereo
	int audioChannels = 2;
	int audioBuffers = 4096;

	// Open SDL_mixer
	if (Mix_OpenAudio(audioRate, audioFormat, audioChannels, audioBuffers))
	{
        return Game::ERROR_PLATFORM;
	}

	// Load the music file and sound effects
	mMusic = Mix_LoadMUS(STC_SND_MUSIC);
	mSoundLine = Mix_LoadWAV(STC_SND_LINE);
	mSoundDrop = Mix_LoadWAV(STC_SND_DROP);

	// Play the music in a loop
	Mix_PlayMusic(mMusic, -1);

    mGame = game;
    return Game::ERROR_NONE;
}

// Return the current system time in milliseconds
long PlatformSdl::getSystemTime()
{
    return SDL_GetTicks();
}

// Process events and notify game
void PlatformSdl::processEvents()
{
    SDL_Event event;

    // Grab events in the queue
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        // On quit game
        case SDL_QUIT:
            mGame->onEventStart(Game::EVENT_QUIT);
            break;
        // On key pressed
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                mGame->onEventStart(Game::EVENT_QUIT);
                break;
            case SDLK_s:
            case SDLK_DOWN:
                mGame->onEventStart(Game::EVENT_MOVE_DOWN);
                break;
            case SDLK_w:
            case SDLK_UP:
                mGame->onEventStart(Game::EVENT_ROTATE_CW);
                break;
            case SDLK_a:
            case SDLK_LEFT:
                mGame->onEventStart(Game::EVENT_MOVE_LEFT);
                break;
            case SDLK_d:
            case SDLK_RIGHT:
                mGame->onEventStart(Game::EVENT_MOVE_RIGHT);
                break;
            case SDLK_SPACE:
                mGame->onEventStart(Game::EVENT_DROP);
                break;
            case SDLK_F5:
                mGame->onEventStart(Game::EVENT_RESTART);
                break;
            case SDLK_F1:
                mGame->onEventStart(Game::EVENT_PAUSE);
                break;
            case SDLK_F2:
                mGame->onEventStart(Game::EVENT_SHOW_NEXT);
                break;
#ifdef STC_SHOW_GHOST_PIECE
            case SDLK_F3:
                mGame->onEventStart(Game::EVENT_SHOW_SHADOW);
                break;
#endif // STC_SHOW_GHOST_PIECE
            default:
                break;
            }
            break;
        // On key released
        case SDL_KEYUP:
            switch (event.key.keysym.sym)
            {
            case SDLK_s:
            case SDLK_DOWN:
                mGame->onEventEnd(Game::EVENT_MOVE_DOWN);
                break;
            case SDLK_a:
            case SDLK_LEFT:
                mGame->onEventEnd(Game::EVENT_MOVE_LEFT);
                break;
            case SDLK_d:
            case SDLK_RIGHT:
                mGame->onEventEnd(Game::EVENT_MOVE_RIGHT);
                break;
#ifdef STC_AUTO_ROTATION
            case SDLK_w:
            case SDLK_UP:
                mGame->onEventEnd(Game::EVENT_ROTATE_CW);
                break;
#endif // STC_AUTO_ROTATION
            default:
                break;
            }
        default:
            break;
        }
    }
}

// Draw a tile from a tetromino
void PlatformSdl::drawTile(int x, int y, int tile, bool shadow)
{
    SDL_Rect recDestine;
    SDL_Rect recSource;

    recDestine.x = (Sint16)x;
    recDestine.y = (Sint16)y;
    recSource.x = (Sint16)(TILE_SIZE * tile);
    recSource.y = (TILE_SIZE + 1) * (shadow? 1 : 0);
    recSource.w = TILE_SIZE + 1;
    recSource.h = TILE_SIZE + 1;
    SDL_BlitSurface(mBmpTiles, &recSource, mScreen, &recDestine);
}

// Draw a number on the given position
void PlatformSdl::drawNumber(int x, int y, long number, int length, int color)
{
    SDL_Rect recDestine;
    SDL_Rect recSource;

    recSource.y = (Sint16)(NUMBER_HEIGHT * color);
    recSource.w = NUMBER_WIDTH;
    recSource.h = NUMBER_HEIGHT;
    recDestine.y = (Sint16)y;

    int pos = 0;
    do
    {
        recDestine.x = (Sint16)(x + NUMBER_WIDTH * (length - pos));
        recSource.x = NUMBER_WIDTH * (Sint16)(number % 10);
        SDL_BlitSurface(mBmpNumbers, &recSource, mScreen, &recDestine);
        number /= 10;
    } while (++pos < length);
}

// Render the state of the game using platform functions
void PlatformSdl::renderGame()
{
    int i, j;

    // Check if the game state has changed, if so redraw
    if (mGame->hasChanged())
    {
        // Draw background
        SDL_BlitSurface(mBmpBack, NULL, mScreen, NULL);

        // Draw preview block
        if (mGame->showPreview())
        {
            for (i = 0; i < Game::TETROMINO_SIZE; ++i)
            {
                for (j = 0; j < Game::TETROMINO_SIZE; ++j)
                {
                    if (mGame->nextBlock().cells[i][j] != Game::EMPTY_CELL)
                    {
                        drawTile(PREVIEW_X + (TILE_SIZE * i),
                                 PREVIEW_Y + (TILE_SIZE * j),
                                 mGame->nextBlock().cells[i][j], false);
                    }
                }
            }
        }
#ifdef STC_SHOW_GHOST_PIECE
        // Draw shadow tetromino
        if (mGame->showShadow() && mGame->shadowGap() > 0)
        {
            for (i = 0; i < Game::TETROMINO_SIZE; ++i)
            {
                for (j = 0; j < Game::TETROMINO_SIZE; ++j)
                {
                    if (mGame->fallingBlock().cells[i][j] != Game::EMPTY_CELL)
                    {
                        drawTile(BOARD_X + (TILE_SIZE * (mGame->fallingBlock().x + i)),
                                 BOARD_Y + (TILE_SIZE * (mGame->fallingBlock().y + mGame->shadowGap() + j)),
                                 mGame->fallingBlock().cells[i][j], true);
                    }
                }
            }
        }
#endif
        // Draw the cells in the board
        for (i = 0; i < Game::BOARD_TILEMAP_WIDTH; ++i)
        {
            for (j = 0; j < Game::BOARD_TILEMAP_HEIGHT; ++j)
            {
                if (mGame->getCell(i, j) != Game::EMPTY_CELL)
                {
                    drawTile(BOARD_X + (TILE_SIZE * i),
                             BOARD_Y + (TILE_SIZE * j),
                             mGame->getCell(i, j), false);
                }
            }
        }

        // Draw falling tetromino
        for (i = 0; i < Game::TETROMINO_SIZE; ++i)
        {
            for (j = 0; j < Game::TETROMINO_SIZE; ++j)
            {
                if (mGame->fallingBlock().cells[i][j] != Game::EMPTY_CELL)
                {
                    drawTile(BOARD_X + (TILE_SIZE * (mGame->fallingBlock().x + i)),
                             BOARD_Y + (TILE_SIZE * (mGame->fallingBlock().y + j)),
                             mGame->fallingBlock().cells[i][j], false);
                }
            }
        }

        // Draw game statistic data
        if (!mGame->isPaused())
        {
            drawNumber(LEVEL_X, LEVEL_Y, mGame->stats().level, LEVEL_LENGTH, Game::COLOR_WHITE);
            drawNumber(LINES_X, LINES_Y, mGame->stats().lines, LINES_LENGTH, Game::COLOR_WHITE);
            drawNumber(SCORE_X, SCORE_Y, mGame->stats().score, SCORE_LENGTH, Game::COLOR_WHITE);

            drawNumber(TETROMINO_X, TETROMINO_L_Y, mGame->stats().pieces[Game::TETROMINO_L], TETROMINO_LENGTH, Game::COLOR_ORANGE);
            drawNumber(TETROMINO_X, TETROMINO_I_Y, mGame->stats().pieces[Game::TETROMINO_I], TETROMINO_LENGTH, Game::COLOR_CYAN);
            drawNumber(TETROMINO_X, TETROMINO_T_Y, mGame->stats().pieces[Game::TETROMINO_T], TETROMINO_LENGTH, Game::COLOR_PURPLE);
            drawNumber(TETROMINO_X, TETROMINO_S_Y, mGame->stats().pieces[Game::TETROMINO_S], TETROMINO_LENGTH, Game::COLOR_GREEN);
            drawNumber(TETROMINO_X, TETROMINO_Z_Y, mGame->stats().pieces[Game::TETROMINO_Z], TETROMINO_LENGTH, Game::COLOR_RED);
            drawNumber(TETROMINO_X, TETROMINO_O_Y, mGame->stats().pieces[Game::TETROMINO_O], TETROMINO_LENGTH, Game::COLOR_YELLOW);
            drawNumber(TETROMINO_X, TETROMINO_J_Y, mGame->stats().pieces[Game::TETROMINO_J], TETROMINO_LENGTH, Game::COLOR_BLUE);

            drawNumber(PIECES_X, PIECES_Y, mGame->stats().totalPieces, PIECES_LENGTH, Game::COLOR_WHITE);
        }

        // Inform the game that we are done with the changed state
        mGame->onChangeProcessed();

        // Swap video buffers
        SDL_Flip(mScreen);
    }

    // Resting game
    SDL_Delay(SLEEP_TIME);
}

// Return a random positive integer number
int PlatformSdl::random()
{
    return rand();
}

void PlatformSdl::onLineCompleted()
{
	Mix_PlayChannel(-1, mSoundLine, 0);
}

void PlatformSdl::onPieceDrop()
{
	Mix_PlayChannel(-1, mSoundDrop, 0);
}

// Release platform allocated resources
void PlatformSdl::end()
{
    // Free all the created surfaces
    SDL_FreeSurface(mBmpTiles);
    SDL_FreeSurface(mBmpBack);
    SDL_FreeSurface(mBmpNumbers);
    SDL_FreeSurface(mScreen);

	// Close SDL_mixer
	Mix_CloseAudio();

    // Shut down SDL
    SDL_Quit();
}
}
