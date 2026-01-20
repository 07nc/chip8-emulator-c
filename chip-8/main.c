#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "SDL.h"
typedef struct{
    SDL_Window* window;
    SDL_Renderer* renderer;
}sdl_t;
//configuration object
typedef struct{
    uint32_t window_width;
    uint32_t window_height;
    uint32_t fgcolor;
    uint32_t bgcolor;
    uint32_t scale;
}config_t;
//emulator states 
typedef enum{
    QUIT,
    RUNNING,
    PAUSED,
}state_t;
typedef struct{
    state_t state;
}chip8_t;
//set initial configuration
bool set_config(config_t*config,int argc,char**argv){
    config->window_height=32;
    config->window_width=64;
    config->bgcolor=0xFF00FFFF;
    config->fgcolor=0x00000000;
    config->scale=20;
    (void)argc;
    (void)argv;
    return true;
}
//initialise SDL
bool init_sdl(sdl_t*sdl,const config_t config){
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)!=0){
        SDL_Log("SDL couldn't be initialized %s\n",SDL_GetError());
        return false;
    }
    //create window
    sdl->window=SDL_CreateWindow("CHIP-8 Emulator",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        config.window_width*config.scale,config.window_height*config.scale,SDL_WINDOW_ALLOW_HIGHDPI);
    if(!sdl->window){
        SDL_Log("SDL Window couldn't be created %s\n",SDL_GetError());
        return false;
    }
    //create renderer
    sdl->renderer=SDL_CreateRenderer(sdl->window,-1,SDL_RENDERER_ACCELERATED);
    if(!sdl->renderer){
        SDL_Log("SDL Renderer couldn't be created %s\n",SDL_GetError());
        return false;
    }
    return true;
}
//clear screen to background color
void clear_screen(sdl_t *sdl,const config_t config){
    const uint8_t r=(config.bgcolor>>24)&0xFF;
    const uint8_t g=(config.bgcolor>>16)&0xFF;
    const uint8_t b=(config.bgcolor>>8)&0xFF;
    const uint8_t a=(config.bgcolor)&0xFF;
    SDL_SetRenderDrawColor(sdl->renderer,r,g,b,a);
    SDL_RenderClear(sdl->renderer);
}
//update screen
void update_screen(sdl_t*sdl){
    SDL_RenderPresent(sdl->renderer);
}

bool init_chip8(chip8_t*chip8){
    chip8->state=RUNNING;
    return true;
}

//quit sdl
void sdl_cleanup(sdl_t*sdl){
    SDL_DestroyRenderer(sdl->renderer);
    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}
void handle_input(chip8_t* chip8){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT:
                chip8->state=QUIT;
                return;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym){
                    case SDLK_ESCAPE:
                        chip8->state=QUIT;
                        return;
                    default:
                        break;
                }
            default:
                break;
        }
    }
}
int main(int argc,char**argv){
    sdl_t sdl={0};
    config_t config={0};
    if(!set_config(&config,argc,argv)){exit(EXIT_FAILURE);}
    chip8_t chip8={0};
    //intialise SDL 
    if(!init_sdl(&sdl,config)){
        exit(EXIT_FAILURE);
    }
    //initial screen clear
    clear_screen(&sdl,config);
    //initialise chip8 
    init_chip8(&chip8);
    if(!init_chip8(&chip8)){
        exit(EXIT_FAILURE);
    }
    while(chip8.state!=QUIT){
        handle_input(&chip8);
        SDL_Delay(16);
        clear_screen(&sdl,config);
        update_screen(&sdl);
    }
    sdl_cleanup(&sdl);
    exit(EXIT_SUCCESS);
}