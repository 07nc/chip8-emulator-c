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
//CHIP-8 machine object
typedef struct{
    state_t state;
    uint8_t ram[4096];
    bool display[64*32]; //pixel matrix
    uint16_t stack[16]; //subroutine stack 
    uint8_t V[16]; //data registers from V0-VF
    uint16_t I; //index register
    uint16_t PC; //program counter register
    uint8_t displayTimer; 
    uint8_t soundTimer;  
    bool keypad[16]; //Hex keypad 0x0-0xF
    char* rom_name; //current ROM file name
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

bool init_chip8(chip8_t*chip8,char rom_name[]){
    uint8_t font[] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    //load font into RAM
    memcpy(&chip8->ram[0],font,sizeof(font));
    //open ROM file
    //set defaults
    chip8->state=RUNNING;
    chip8->PC=0x200; //ROM starts at 0x200

    return true;
}

//quit sdl
void sdl_cleanup(sdl_t*sdl){
    SDL_DestroyRenderer(sdl->renderer);
    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}

//event handling
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
    init_chip8(&chip8,argv[1]);
    if(!init_chip8(&chip8,argv[1])){
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