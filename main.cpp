#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <vector>
#include <cmath>
#include "defs.h"
#include "graphics.h"
#include "characters.h"
#include "intro.h"
#include "result.h"

using namespace std;

void waitUntilKeyPressed(){
    SDL_Event e;
    while (true){
        if (SDL_PollEvent(&e) != 0 && (e.type == SDL_KEYDOWN || e.type == SDL_QUIT))
            return;
        SDL_Delay(10);
    }
}

int main(int argc, char *argv[])
{
    bool restartGame = true;
    bool resultShow = false;
    while(restartGame){
        restartGame = false;

        //fixed do not touch
        Graphics graphic;
        graphic.init();
        srand(time(0));

        //music
        Mix_Music *gmusic = graphic.loadMusic("Ticktack-ILLIT-16507480.mp3");
        graphic.play(gmusic);

        //pacman show up
        eatingPac pac;
        SDL_Texture* pacTexture = graphic.loadTexture(PACMAN_FILE);
        pac.init(pacTexture, PAC_FRAMES, PACMAN_CLIPS);
        //ghost show up
        eyeroll xanh;
        SDL_Texture* ghostTextureB = graphic.loadTexture("maxanh-01.png");
        xanh.init(ghostTextureB, GHOST_FRAMES, GHOST_CLIPS);
        eyeroll fire;
        SDL_Texture* ghostTextureR = graphic.loadTexture("mado-01.png");
        fire.init(ghostTextureR, GHOST_FRAMES, GHOST_CLIPS);
        eyeroll cam;
        SDL_Texture* ghostTextureO = graphic.loadTexture("macam-01.png");
        cam.init(ghostTextureO, GHOST_FRAMES, GHOST_CLIPS);
        eyeroll hong;
        SDL_Texture* ghostTextureP = graphic.loadTexture("mahong-01.png");
        hong.init(ghostTextureP, GHOST_FRAMES, GHOST_CLIPS);

        //ghost run
        ghost blue(80, 40);
        ghost red(720, 80);
        ghost orange(80, 760);
        ghost pink(720, 760);
        vector<ghost*> ghosts = {&red, &blue, &orange, &pink};
        vector<eyeroll*> ghostAnimations = {&fire, &xanh, &cam, &hong};

        vector<pair<int, int>> ghostPath;

        //main render
        Pac run;
        bool quit = false;
        SDL_Event e;

        outro* showResult = new outro();
        //intro
        bool quitGame = showIntro(graphic.renderer, gmusic);
        if (quitGame) {
            Mix_HaltMusic();
            if (gmusic != nullptr) Mix_FreeMusic(gmusic);
            graphic.quit();
            return 0;
        }

        bool introRunning = true;
        while (introRunning) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) exit(0);
                if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN)
                    introRunning = false;
            }
            SDL_Delay(10);
        }

        while (!quit && !gameOver(run)){
            graphic.prepareScene();

            while (SDL_PollEvent(&e)){
                if (e.type == SDL_QUIT) quit = true;
            }

            //handle keyboard of pacman moving
            const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

            if (currentKeyStates[SDL_SCANCODE_UP]) {
                run.setDirection(UP);
            }
            else if (currentKeyStates[SDL_SCANCODE_DOWN]) {
                run.setDirection(DOWN);
            }
            else if (currentKeyStates[SDL_SCANCODE_LEFT]) {
                run.setDirection(LEFT);
            }
            else if (currentKeyStates[SDL_SCANCODE_RIGHT]) {
                run.setDirection(RIGHT);
            } else run.setDirection(NONE);

            run.Move(MAP);

            //the player has won
            vector<vector<int>> mapVector(MAP_H, vector<int>(MAP_W, 0));
            for (int i = 0; i<MAP_H; i++){
                for (int j = 0; j<MAP_W; j++){
                    mapVector[i][j] = MAP[i][j];
                }
            }

            int cnt = 0;

            for (const auto& row : mapVector) {
                cnt += count(row.begin(), row.end(), 2);
            }
            if(cnt == 0){
                showResult->won = true;
                quitGame = showResult->showResult(graphic.renderer);
                quit = true;
                resultShow = true;
            }

            //this is for ghost
            Uint32 currentTime = SDL_GetTicks64();

            for (auto& g : ghosts) {
                 if (g->heuristic(run.x, run.y) < chase) {
                    g->pathCalculate = true;

                    int targetTileX = (run.x + Psize / 2) / tile;
                    int targetTileY = (run.y + Psize / 2) / tile;

                    if ((targetTileX != g->lastTargetX || targetTileY != g->lastTargetY) &&
                        currentTime > g->lastPathCalculation + g->pathCalculationDelay) {

                        g->path = g->findPath(targetTileX * tile, targetTileY * tile);
                        g->pathIndex = 0;
                        g->lastPathCalculation = currentTime;
                        g->lastTargetX = targetTileX;
                        g->lastTargetY = targetTileY;
                    }

                    if (!g->path.empty() && g->pathIndex < g->path.size()) {
                        int nextX = g->path[g->pathIndex].first;
                        int nextY = g->path[g->pathIndex].second;

                        int dx = nextX - g->x;
                        int dy = nextY - g->y;
                        int distSquared = dx * dx + dy * dy;

                        int prevX = g->x;
                        int prevY = g->y;

                        if (distSquared <= g->speed * g->speed + 4) {
                            g->x = nextX;
                            g->y = nextY;
                            g->pathIndex++;
                            g->stuckCounter = 0;
                        } else {
                            float dist = sqrt((float)distSquared);
                            float normDx = dx / dist;
                            float normDy = dy / dist;

                            g->x += round(g->speed * normDx);
                            g->y += round(g->speed * normDy);

                            if (g->x == prevX && g->y == prevY) {
                                g->stuckCounter++;
                                if (g->stuckCounter > 3) {
                                    g->path.clear();
                                    g->stuckCounter = 0;
                                }
                            } else {
                                g->stuckCounter = 0;
                            }
                        }
                    } else {
                        g->Move();
                    }
                } else {
                    g->pathCalculate = false;
                    g->Move();
                }
            }

            for (const auto& g : ghosts) {
                if (checkCollision(run.x, run.y, Psize, g->x, g->y, Gsize)) {
                    SDL_Log("YOU DIED!");
                    DIED++;
                    quit = true;
                    break;
                }
            }


            //set frames
            pac.tick();
            for (auto& anim : ghostAnimations) anim->tick();
            //draw map
            graphic.drawMap(MAP);

            graphic.renderP(run.x, run.y, pac);
            graphic.renderG(blue.x, blue.y, xanh);
            graphic.renderG(orange.x, orange.y, cam);
            graphic.renderG(red.x, red.y, fire);
            graphic.renderG(pink.x, pink.y, hong);

            graphic.presentScene();

            SDL_Delay(100);
        }
        if(!resultShow) quitGame = showResult->showResult(graphic.renderer);

        Mix_HaltMusic();
        if (gmusic != nullptr) Mix_FreeMusic(gmusic);

        //fixed do not touch
        graphic.quit();

        if (!quitGame) {
            restartGame = true;
        }
    }
    return 0;
 }
