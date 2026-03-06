#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
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
    uint32_t clock_rate_hz; //instructions per second
}config_t;

//emulator states 
typedef enum{
    QUIT,
    RUNNING,
    PAUSED,
}state_t;

//CHIP-8 instruction
typedef struct{
    uint16_t opcode; //2 byte opcode
    uint16_t NNN; //2 byte address/constant
    uint8_t NN; //8 bit constant
    uint8_t N; //4 bit constant
    uint8_t X; //4 bit register identifier 
    uint8_t Y; //4 bit register identifier

}instruction_t;

//CHIP-8 machine object
typedef struct{
    state_t state;
    uint8_t ram[4096];
    bool display[64*32]; //pixel matrix
    uint16_t stack[16]; //subroutine stack 
    uint8_t V[16]; //data registers from V0-VF
    uint16_t I; //index register
    uint16_t PC; //program counter register
    uint16_t* SP; //stack pointer
    uint8_t delayTimer; 
    uint8_t soundTimer;  
    bool keypad[16]; //Hex keypad 0x0-0xF
    const char* rom_name; //current ROM file name
    instruction_t instruction; //current instruction
}chip8_t;

//set initial configuration
bool set_config(config_t*config,int argc,char**argv){
    config->window_height=32;
    config->window_width=64;
    config->bgcolor=0x00000000;
    config->fgcolor=0xFFFFFFFF;
    config->scale=10;
    config->clock_rate_hz=700;
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
        config.window_width*config.scale,config.window_height*config.scale,0);
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
void clear_screen(sdl_t *sdl,const config_t *config){
    const uint8_t r=(config->bgcolor>>24)&0xFF;
    const uint8_t g=(config->bgcolor>>16)&0xFF;
    const uint8_t b=(config->bgcolor>>8)&0xFF;
    const uint8_t a=(config->bgcolor)&0xFF;
    SDL_SetRenderDrawColor(sdl->renderer,r,g,b,a);
    SDL_RenderClear(sdl->renderer);
}

//update screen
void update_screen(const sdl_t*sdl, const chip8_t*chip8, const config_t*config){
    SDL_Rect rect={.x=0,.y=0,.h=config->scale,.w=config->scale};

    //intialise rgba values from config bgcolor and fgcolor
    const uint8_t bg_r=(config->bgcolor>>24)&0xFF;
    const uint8_t bg_g=(config->bgcolor>>16)&0xFF;
    const uint8_t bg_b=(config->bgcolor>>8)&0xFF;
    const uint8_t bg_a=(config->bgcolor)&0xFF;

    const uint8_t fg_r=(config->fgcolor>>24)&0xFF;
    const uint8_t fg_g=(config->fgcolor>>16)&0xFF;
    const uint8_t fg_b=(config->fgcolor>>8)&0xFF;
    const uint8_t fg_a=(config->fgcolor)&0xFF;

    //loop through display array and draw filled rectangles if pixel is set to 1
    for(uint32_t i=0;i<sizeof(chip8->display);i++){
        //y = i/window_width
        //x = i%window_width
        rect.y=(i/config->window_width)*config->scale;
        rect.x=(i%config->window_width)*config->scale;
        if(chip8->display[i]){
            //if pixel is on, draw rect with foreground color
            SDL_SetRenderDrawColor(sdl->renderer,fg_r,fg_g,fg_b,fg_a);
            SDL_RenderFillRect(sdl->renderer,&rect);
            SDL_SetRenderDrawColor(sdl->renderer,bg_r,bg_g,bg_b,bg_a);
            SDL_RenderDrawRect(sdl->renderer,&rect);
        } else{
            //if pixel is off, draw rect with background color
            SDL_SetRenderDrawColor(sdl->renderer,bg_r,bg_g,bg_b,bg_a);
            SDL_RenderFillRect(sdl->renderer,&rect);
        }
    }

    SDL_RenderPresent(sdl->renderer);

}

bool init_chip8(chip8_t*chip8, const char rom_name[]){
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
    const uint32_t rom_start=0x200;
    //load font into RAM
    memcpy(&chip8->ram[0],font,sizeof(font));
    //open ROM file
    FILE*rom=fopen(rom_name,"rb");
    if(!rom){
        SDL_Log("ROM File %s is invalid or doesn't exist \n",rom_name);
        return false;
    }
    //get ROM size
    fseek(rom,0,SEEK_END);
    const size_t rom_size=ftell(rom);
    rewind(rom);
    if(rom_size>sizeof chip8->ram-rom_start){
        SDL_Log("ROM File size exceeds max size allowed\n");
        return false;
    }
    if(fread(&chip8->ram[rom_start],rom_size,1,rom)!=1){
        SDL_Log("Could not load ROM File to memory %s \n",rom_name);
        return false;
    }
    fclose(rom);
    //set defaults
    chip8->state=RUNNING;
    chip8->PC=rom_start; //Program Counter points to where ROM begins
    chip8->rom_name=rom_name;
    chip8->SP=&chip8->stack[0]; 
    return true;
}

//quit sdl
void sdl_cleanup(sdl_t*sdl){
    SDL_DestroyRenderer(sdl->renderer);
    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}

//keypad layout
// CHIP8    QWERTY
// 123C     1234
// 456D     qwer
// 789E     asdf
// A0BF     zxcv

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
                    //quit
                    case SDLK_ESCAPE:
                        chip8->state=QUIT;
                        return;
                    //pause
                    case SDLK_SPACE:
                        if(chip8->state==RUNNING){
                        chip8->state=PAUSED;
                        puts("PAUSED");
                        }
                        else chip8->state=RUNNING;
                        return;

                    //update chip8 keypad array on keydown
                    case SDLK_1:
                        chip8->keypad[0x1]=true; 
                        break;
                    case SDLK_2:
                        chip8->keypad[0x2]=true; 
                        break;
                    case SDLK_3:
                        chip8->keypad[0x3]=true; 
                        break;
                    case SDLK_4:
                        chip8->keypad[0xC]=true; 
                        break;

                    case SDLK_q:
                        chip8->keypad[0x4]=true; 
                        break;
                    case SDLK_w:
                        chip8->keypad[0x5]=true; 
                        break;
                    case SDLK_e:
                        chip8->keypad[0x6]=true; 
                        break;
                    case SDLK_r:
                        chip8->keypad[0xD]=true; 
                        break;

                    case SDLK_a:
                        chip8->keypad[0x7]=true; 
                        break;
                    case SDLK_s:
                        chip8->keypad[0x8]=true; 
                        break;
                    case SDLK_d:
                        chip8->keypad[0x9]=true; 
                        break;
                    case SDLK_f:
                        chip8->keypad[0xE]=true; 
                        break;

                    case SDLK_z:
                        chip8->keypad[0xA]=true; 
                        break;
                    case SDLK_x:
                        chip8->keypad[0x0]=true; 
                        break;
                    case SDLK_c:
                        chip8->keypad[0xB]=true; 
                        break;
                    case SDLK_v:
                        chip8->keypad[0xF]=true; 
                        break;

                    default:
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym){
                    //update chip8 keypad array on keyup
                    case SDLK_1:
                        chip8->keypad[0x1]=false; 
                        break;
                    case SDLK_2:
                        chip8->keypad[0x2]=false; 
                        break;
                    case SDLK_3:
                        chip8->keypad[0x3]=false; 
                        break;
                    case SDLK_4:
                        chip8->keypad[0xC]=false; 
                        break;

                    case SDLK_q:
                        chip8->keypad[0x4]=false; 
                        break;
                    case SDLK_w:
                        chip8->keypad[0x5]=false; 
                        break;
                    case SDLK_e:
                        chip8->keypad[0x6]=false; 
                        break;
                    case SDLK_r:
                        chip8->keypad[0xD]=false; 
                        break;

                    case SDLK_a:
                        chip8->keypad[0x7]=false; 
                        break;
                    case SDLK_s:
                        chip8->keypad[0x8]=false; 
                        break;
                    case SDLK_d:
                        chip8->keypad[0x9]=false; 
                        break;
                    case SDLK_f:
                        chip8->keypad[0xE]=false; 
                        break;

                    case SDLK_z:
                        chip8->keypad[0xA]=false; 
                        break;
                    case SDLK_x:
                        chip8->keypad[0x0]=false; 
                        break;
                    case SDLK_c:
                        chip8->keypad[0xB]=false; 
                        break;
                    case SDLK_v:
                        chip8->keypad[0xF]=false; 
                        break;

                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}

void update_timers(chip8_t*chip8){
    if(chip8->delayTimer>0){
        chip8->delayTimer--;
    }
    if(chip8->soundTimer>0){
        chip8->soundTimer--;
    }
}

void emulate_instruction(chip8_t*chip8, config_t*config){
    //get the next opcode from RAM 
    //RAM is an array of 8 bits
    //read the first 8 bits, left shift by 8 then OR with next 8 bits
    chip8->instruction.opcode=chip8->ram[chip8->PC]<<8 | chip8->ram[chip8->PC+1];
    //each instruction is 2 bytes so increment the program counter by 2
    chip8->PC+=2;
    chip8->instruction.NNN=chip8->instruction.opcode&0xFFF;
    chip8->instruction.NN=chip8->instruction.opcode&0xFF;
    chip8->instruction.N=chip8->instruction.opcode&0xF;
    chip8->instruction.X=(chip8->instruction.opcode>>8)&0xF;
    chip8->instruction.Y=(chip8->instruction.opcode>>4)&0xF;

    switch ((chip8->instruction.opcode>>12)&0xF) //First digit
    {

    case 0x0:
        if(chip8->instruction.NN==0xE0){
            //clear screen - 0x00E0
            memset(&chip8->display[0],0,sizeof(chip8->display));
        }
        else if(chip8->instruction.NN==0xEE){
            //return from subroutine - 0x00EE
            //come back to address stored at top of stack
            chip8->PC=*--chip8->SP;
        }else{printf("Unimplemented ");}
        break;

    case 0x1:
        //Jump to NNN - 0x1NNN
        //Program counter is set to NNN 
        chip8->PC=chip8->instruction.NNN;
        break;

    case 0x2:
        //Call subroutine at NNN - 0x2NNN
        *chip8->SP++=chip8->PC; //push current address to stack and increment stack pointer
        chip8->PC=chip8->instruction.NNN; //move PC to NNN
        break;

    case 0x3:
        //Skips the next instruction if VX equals NN - 0x3XNN
        if(chip8->V[chip8->instruction.X]==chip8->instruction.NN){
            chip8->PC+=2; 
        }
        break;

    case 0x4:
        //Skips the next instruction if VX does not equal NN - 0x4XNN
        if(chip8->V[chip8->instruction.X]!=chip8->instruction.NN){
            chip8->PC+=2; 
        }
        break;

    case 0x5:
        if(chip8->instruction.N!=0){
            break;
        }
        //Skips the next instruction if VX equals VY - 0x5XY0
        if(chip8->V[chip8->instruction.X]==chip8->V[chip8->instruction.Y]){
            chip8->PC+=2; 
        }
        break;

    case 0x6:
        //Set Data Register VX to NN - 0x6XNN
        chip8->V[chip8->instruction.X]=chip8->instruction.NN;
        break;

    case 0x7:
        // Add NN to VX - 0x7XNN
        chip8->V[chip8->instruction.X]+=chip8->instruction.NN;
        break;
        
    case 0x8:
        if(chip8->instruction.N==0){
            //Sets VX to value of VY - 0x8XY0
            chip8->V[chip8->instruction.X]=chip8->V[chip8->instruction.Y];
        }

        else if(chip8->instruction.N==1){
            //Sets VX to VX | VY - 0x8XY1
            chip8->V[chip8->instruction.X]|=chip8->V[chip8->instruction.Y];
        }

        else if(chip8->instruction.N==2){
            //Sets VX to VX & VY - 0x8XY2
            chip8->V[chip8->instruction.X]&=chip8->V[chip8->instruction.Y];
        }

        else if(chip8->instruction.N==3){
            //Sets VX to VX xor VY - 0x8XY3
            chip8->V[chip8->instruction.X]^=chip8->V[chip8->instruction.Y];
        }

        else if(chip8->instruction.N==4){
            //Adds VY to VX - 0x8XY4
            uint16_t temp= chip8->V[chip8->instruction.X]+chip8->V[chip8->instruction.Y];
            //VF is set to 1 if overflow, and to 0 if there's no overflow
            chip8->V[chip8->instruction.X]+=chip8->V[chip8->instruction.Y];
            if(temp>255){
                chip8->V[0xF]=1;
            }
            else{
                chip8->V[0xF]=0;
            }
        }

        else if(chip8->instruction.N==5){
            //Subtracts VY from VX - 0x8XY5
            //VF is set to 1 if VX >= VY and 0 if not
            uint8_t temp=chip8->V[chip8->instruction.X];
            chip8->V[chip8->instruction.X]-=chip8->V[chip8->instruction.Y];
            if( temp>=chip8->V[chip8->instruction.Y]){
                 chip8->V[0xF]=1;
            }
            else{
                chip8->V[0xF]=0;
            }            
        }

        else if(chip8->instruction.N==6){
            uint8_t temp=chip8->V[chip8->instruction.X];
            //Right Shifts VX by 1 - 0x8XY6
            chip8->V[chip8->instruction.X]>>=1;
            //VF is set to LSB of VX prior to shifting
            chip8->V[0xF]=(temp&1);
        }

        else if(chip8->instruction.N==7){
            //Sets VX to VY-VX - 0x8XY7
            //VF set to 1 if VY >= VX
            uint8_t temp=chip8->V[chip8->instruction.X];
            chip8->V[chip8->instruction.X]=chip8->V[chip8->instruction.Y]-chip8->V[chip8->instruction.X];
            if( chip8->V[chip8->instruction.Y]>=temp){
                 chip8->V[0xF]=1;
            }
            else{
                chip8->V[0xF]=0;
            }            
        }       

        else if(chip8->instruction.N==0xE){
            uint8_t temp=chip8->V[chip8->instruction.X];
            //Left Shifts VX by 1 - 0x8XYE
            chip8->V[chip8->instruction.X]<<=1;
            //If MSB of VX prior to shifting is set, VX=1, else VX=0
            chip8->V[0xF]=(temp>>7)&1;
        }  
        break;

    case 0x9:
        //Skips the next instruction if VX does not equal VY - 0x9XY0
        if(chip8->V[chip8->instruction.X]!=chip8->V[chip8->instruction.Y]){
            chip8->PC+=2;
        }
        break;

    case 0xA:
        //Set Index Register to NNN - 0xANNN
        chip8->I=chip8->instruction.NNN;
        break;

    case 0xB:
        //Jump to V0+NNN - 0xBNNN
        chip8->PC=chip8->V[0]+chip8->instruction.NNN;
        break;

    case 0xC:
        //VX = random number from 0 to 255 & NN
        chip8->V[chip8->instruction.X]=rand()%256 & chip8->instruction.NN;
        break;
        
    case 0xD:{
        //Draw Sprite at (X,Y) of height N from memory address I - 0xDXYN
        //To draw we XOR display matrix with the sprite matrix
        //VF is set if a set display pixel switches OFF
        //first, we need wrap the coordinates around the screen
        uint8_t X_coord=chip8->V[chip8->instruction.X] % config->window_width;
        uint8_t Y_coord=chip8->V[chip8->instruction.Y]% config->window_height;
        chip8->V[0xF] = 0;
        //then iterate over N rows of the sprite
        for(int i=0;i<chip8->instruction.N;i++){
            //8 bit sprite data 
            const uint8_t sprite_bin = chip8->ram[chip8->I+i];
            for(int j=7;j>=0;j--){
                uint8_t target_X=X_coord+7-j;
                bool *pixel=&chip8->display[(Y_coord+i)*config->window_width+target_X];
                //if display pixel is on and the sprite bit is on then VF is set to 1
                if(*pixel&&(sprite_bin&(1<<j))){
                    chip8->V[0xF]=1;
                }
                //xor display pixel with current sprite bit
                *pixel ^= (bool)(sprite_bin&(1<<j));
                if(target_X+1>=config->window_width)break;
            }
            if(Y_coord+i+1>=config->window_height)break;
        }
        break;}
    
    case 0xE:
        if(chip8->instruction.NN==0x9E){
            //Skip next instruction if key stored in VX is pressed - 0xEX9E
            if(chip8->keypad[chip8->V[chip8->instruction.X]])
            chip8->PC+=2;
        }
        else if(chip8->instruction.NN==0xA1){
            //Skip next instruction if key stored in VX is not pressed - 0xEXA1
            if(!chip8->keypad[chip8->V[chip8->instruction.X]])
            chip8->PC+=2;
        }
        break;
    
    case 0xF:
        switch (chip8->instruction.NN)
        {
        case 0x0A:
            //FX0A 
            //VX = get_key(), halts all instructions until next key event
            //delay and sound timers continue processing
            bool key_pressed=false;
            for(uint8_t i=0;i<sizeof chip8->keypad;i++){
                if(chip8->keypad[i]){
                    chip8->V[chip8->instruction.X]=i; //store current key (0-15)
                    key_pressed=1;
                }
            }
            //if no key is pressed then keep repeating the same instruction
            if(!key_pressed)chip8->PC-=2; 
            break;

        case 0x1E:
            //FX1E - Add VX to I
            chip8->I+=chip8->V[chip8->instruction.X];
            break;

        case 0x15:
            //FX15 - Set delay timer to VX
            chip8->delayTimer=chip8->V[chip8->instruction.X];
            break;

        case 0x07:
            //FX07 - Set VX to delay timer 
            chip8->V[chip8->instruction.X]=chip8->delayTimer;
            break;
            
        case 0x18:
            //FX18 - Set sound timer to VX
            chip8->soundTimer=chip8->V[chip8->instruction.X];
            break;

        case 0x29:
            //FX29 - Set I to location of sprite for character in VX
            chip8->I=chip8->V[chip8->instruction.X]*5;
            break;

        case 0x33:
            //FX33 - Store BCD representation of VX at
            //location I - hundreds digit
            //location I+1 - tens digit
            //location I+2 - ones digit
            chip8->ram[chip8->I]=chip8->V[chip8->instruction.X]/100;
            chip8->ram[chip8->I+1]=(chip8->V[chip8->instruction.X]/10)%10;
            chip8->ram[chip8->I+2]=(chip8->V[chip8->instruction.X])%10;
            break;

        case 0x55:
            //FX55 - register dump - stores from V0 to VX in memory starting at location I 
            for(uint8_t i=0;i<=chip8->instruction.X;i++){
                chip8->ram[chip8->I+i]=chip8->V[i];
            }
            break;

        case 0x65:
            //FX65 - register load - fills from V0 to VX with values in memory starting at location I 
            for(uint8_t i=0;i<=chip8->instruction.X;i++){
                chip8->V[i]=chip8->ram[chip8->I+i];
            }
            break;
        default:
            break;
        }
        break;
    default:
        printf("Unimplemented ");
        break;
    }
}

int main(int argc,char**argv){
    if(argc<2){
        fprintf(stderr,"Usage: %s <rom_name>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    sdl_t sdl={0};
    config_t config={0};

    //set initial configuration
    if(!set_config(&config,argc,argv)){exit(EXIT_FAILURE);}

    chip8_t chip8={0};

    //intialise SDL 
    if(!init_sdl(&sdl,config)){
        exit(EXIT_FAILURE);
    }

    //seed random number generator
    srand(time(NULL));

    //initial screen clear
    clear_screen(&sdl,&config);
    const char* rom_name=argv[1];

    //initialise chip8 
    if(!init_chip8(&chip8,rom_name)){
        exit(EXIT_FAILURE);
    }
    
    //this loop runs at 60Hz
    while(chip8.state!=QUIT){

        //handle user input 
        handle_input(&chip8);

        //pause emulation
        if(chip8.state==PAUSED){continue;}

        //get time before emulating instructions
        const uint64_t start_time=SDL_GetPerformanceCounter();
        
        //emulate instructions for one frame
        //we need to emulate instructions clock_rate_hz times per second
        //and the while loop runs 60 times per second
        //so in each loop we emulate clock_rate_hz/60 instructions 
        for(uint32_t i=0;i<config.clock_rate_hz/60;i++){
            emulate_instruction(&chip8,&config);    
            printf("%04X\n",chip8.instruction.opcode);
        }

        //get time after emulating instructions
        const uint64_t end_time=SDL_GetPerformanceCounter();

        //get time taken to emulate instructions
        const double time_taken=(double)((end_time-start_time)*1000)/SDL_GetPerformanceFrequency();

        //make sure that the while loop lasts 16.67 ms 
        //if time_taken>16.67 then we dont delay, else we delay for the remaining time
        SDL_Delay(time_taken>16.67f?0:16.67f-time_taken);

        //update delay and sound timers at 60Hz 
        update_timers(&chip8);
        
        //update SDL screen using chip8 display array
        update_screen(&sdl,&chip8,&config);
    }

    sdl_cleanup(&sdl);
    exit(EXIT_SUCCESS);
}