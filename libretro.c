#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __CELLOS_LV2__
//#include <malloc.h>
#endif

#ifdef _MSC_VER
//#define snprintf _snprintf
//#pragma pack(1)
#endif

#include "libretro.h"

#include "platform.h"
#include "memory.h"
#include "atari.h"
#include "config.h"
#include "monitor.h"
#include "log.h"
#ifdef SOUND
#include "sound.h"
#endif
#include "screen.h"
#include "colours.h"
#include "cfg.h"
#include "devices.h"
#include "input.h"
#include "rtime.h"
#include "sio.h"
#include "cassette.h"
#include "pbi.h"
#include "antic.h"
#include "gtia.h"
#include "pia.h"
#include "pokey.h"
#include "ide.h"
#include "cartridge.h"
#include "ui.h"
#include "akey.h"

typedef struct {
	int fire;
	int start;
	int pause;
	int reset;
	int up;
	int down;
	int left;
	int right;
} ATR5200ControllerState;

char bios_filename[2048];
static bool failed_init;
static int num_cont = 4;

unsigned short frame_buffer[384*240];

void ATR800WriteSoundBuffer(uint8_t *buffer, unsigned int len);
ATR5200ControllerState controllerStates[4];

static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

void retro_set_environment(retro_environment_t cb) { environ_cb = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

ATR5200ControllerState controllerStateForPlayer(unsigned long playerNum)
{
	ATR5200ControllerState state = {0,0,0,0,0,0,0,0};
	if(playerNum < 4) {
		state = controllerStates[playerNum];
	}
	return state;
}

//Atari800 platform calls
int PLATFORM_Initialise(int *argc, char *argv[])
{
#ifdef SOUND
	Sound_Initialise(argc, argv);
#endif
	
	return TRUE;
}

int PLATFORM_Exit(int run_monitor)
{
	Log_flushlog();
	
	if (run_monitor && MONITOR_Run())
		return TRUE;
	
#ifdef SOUND
	Sound_Exit();
#endif
	
	return FALSE;
}

// believe these are used for joystick/input or something
// they get called off of the frame call
int PLATFORM_PORT(int num)
{
/*	if(num < 4 && num >= 0) {
		ATR5200ControllerState state = [currentCore controllerStateForPlayer:num];
		if(state.up == 1 && state.left == 1) {
			return INPUT_STICK_UL;
		}
		else if(state.up == 1 && state.right == 1) {
			return INPUT_STICK_UR;
		}
		else if(state.up == 1) {
			printf("UP");
			return INPUT_STICK_FORWARD;
		}
		else if(state.down == 1 && state.left == 1) {
			return INPUT_STICK_LL;
		}
		else if(state.down == 1 && state.right == 1) {
			printf("Left-right");
			return INPUT_STICK_LR;
		}
		else if(state.down == 1) {
			printf("DOWN");
			return INPUT_STICK_BACK;
		}
		else if(state.left == 1) {
			printf("Left");
			return INPUT_STICK_LEFT;
		}
		else if(state.right == 1) {
			printf("Right");
			return INPUT_STICK_RIGHT;
		}
		return INPUT_STICK_CENTRE;
	}*/
	return 0xff;
}

int PLATFORM_TRIG(int num)
{
	if(num < 4 && num >= 0) {
		ATR5200ControllerState state = controllerStateForPlayer(num);
		if(state.fire == 1) {
		}
		return state.fire == 1 ? 0 : 1;
	}
	return 1;
}

// Looks to be called when the atari UI is on screen
// in ui.c & ui_basic.c
int PLATFORM_Keyboard(void)
{
	return 0;
}

// maybe we can update our RGB buffer in this?
void PLATFORM_DisplayScreen(void)
{
}

int Atari_POT(int num)
{
	int val;
//	cont_cond_t *cond;
	
	if (Atari800_machine_type != Atari800_MACHINE_5200) {
		if (0 /*emulate_paddles*/) {
//			if (num + 1 > num_cont) return(228);
//			
//			cond = &mcond[num];
//			val = cond->joyx;
//			val = val * 228 / 255;
//			if (val > 227) return(1);
//			return(228 - val);
		}
		else {
			return(228);
		}
	}
	else {	/* 5200 version:
			 *
			 * num / 2: which controller
			 * num & 1 == 0: x axis
			 * num & 1 == 1: y axis
			 */
		
		if (num / 2 + 1 > num_cont) return(INPUT_joy_5200_center);
		ATR5200ControllerState state = controllerStateForPlayer(num/2);
//		cond = &mcond[num / 2];
		if(num & 1) { // y-axis
			if(state.up)
				val = 255;
			else if(state.down)
				val = 0;
			else
				val = 127;
		}
		else { // x-axis
			if(state.right)
				val = 255;
			else if(state.left)
				val = 0;
			else
				val = 127;
		}
//		val = (num & 1) ? cond->joyy : cond->joyx;
		
		/* normalize into 5200 range */
		if (val == 127) return(INPUT_joy_5200_center);
		if (val < 127) {
			/*val -= INPUT_joy_5200_min;*/
			val = val * (INPUT_joy_5200_center - INPUT_joy_5200_min) / 127;
			return(val + INPUT_joy_5200_min);
		}
		else {
			val = val * INPUT_joy_5200_max / 255;
			if (val < INPUT_joy_5200_center)
				val = INPUT_joy_5200_center;
			return(val);
		}
	}
}

int16_t convertSample(uint8_t sample)
{
	float floatSample = (float)sample / 255;
	return (int16_t)(floatSample * 65535 - 32768);
}

void ATR800WriteSoundBuffer(uint8_t *buffer, unsigned int len) {
	int samples = len / sizeof(uint8_t);
	unsigned long newLength = len * sizeof(int16_t);
	int16_t *newBuffer = malloc(len * sizeof(int16_t));
	int16_t *dest = newBuffer;
	uint8_t *source = buffer;
	for(int i = 0; i < samples; i++) {
		*dest = convertSample(*source);
		dest++;
		source++;
	}
	//[ringBuffer write:newBuffer maxLength:newLength];
	free(newBuffer);
}


static void update_input()
{

    if (!input_poll_cb)
        return;
    
    input_poll_cb();
	
	//do input
}

/************************************
 * libretro implementation
 ************************************/

static struct retro_system_av_info g_av_info;

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
	info->library_name = "Atari800";
	info->library_version = "2.2.1";
	info->need_fullpath = true;
	info->valid_extensions = "a52|A52|bin|BIN";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    memset(info, 0, sizeof(*info));
    // Just assume NTSC for now. TODO: Verify FPS.
    info->timing.fps            = 60;
    info->timing.sample_rate    = 22050;
    info->geometry.base_width   = 384;
    info->geometry.base_height  = 240;
    info->geometry.max_width    = 384;
    info->geometry.max_height   = 240;
    info->geometry.aspect_ratio = 4.0 / 3.0;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

size_t retro_serialize_size(void) 
{ 
	//return STATE_SIZE;
	return 0;
}

bool retro_serialize(void *data, size_t size)
{ 
   //if (size != STATE_SIZE)
   return false;
}

bool retro_unserialize(const void *data, size_t size)
{
   //if (size != STATE_SIZE)
   return false;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

bool retro_load_game(const struct retro_game_info *info)
{
	if (failed_init)
		return false;
	
    const char *full_path;
    
    full_path = info->path;

	strcpy(CFG_5200_filename, bios_filename);
	
	//Set our colors
	Atari800_tv_mode = Atari800_TV_NTSC;
	Colours_PreInitialise();
	
	//Setup machine
	Atari800_machine_type = Atari800_MACHINE_5200;
	MEMORY_ram_size = 16;
	int test = 0;
	int *argc = &test;
	char *argv[] = {};
	if (
#if !defined(BASIC) && !defined(CURSES_BASIC)
		!Colours_Initialise(argc, argv) ||
#endif
		!Devices_Initialise(argc, argv)
		|| !RTIME_Initialise(argc, argv)
#ifdef IDE
		|| !IDE_Initialise(argc, argv)
#endif
		|| !SIO_Initialise (argc, argv)
		|| !CASSETTE_Initialise(argc, argv)
		|| !PBI_Initialise(argc,argv)
#ifdef VOICEBOX
		|| !VOICEBOX_Initialise(argc, argv)
#endif
#ifndef BASIC
		|| !INPUT_Initialise(argc, argv)
#endif
#ifdef XEP80_EMULATION
		|| !XEP80_Initialise(argc, argv)
#endif
#ifdef AF80
		|| !AF80_Initialise(argc, argv)
#endif
#ifdef NTSC_FILTER
		|| !FILTER_NTSC_Initialise(argc, argv)
#endif
#if SUPPORTS_CHANGE_VIDEOMODE
		|| !VIDEOMODE_Initialise(argc, argv)
#endif
#ifndef DONT_DISPLAY
		/* Platform Specific Initialisation */
		|| !PLATFORM_Initialise(argc, argv)
#endif
		|| !Screen_Initialise(argc, argv)
		/* Initialise Custom Chips */
		|| !ANTIC_Initialise(argc, argv)
		|| !GTIA_Initialise(argc, argv)
		|| !PIA_Initialise(argc, argv)
		|| !POKEY_Initialise(argc, argv)
		) {
		printf("Failed to initialize part of atari800");
		return false;
	}
	
	// this gets called again, maybe we can skip this first one?
	if(!Atari800_InitialiseMachine()) {
		printf("** Failed to initialize machine");
		return false;
	}
	
	/* Install requested ROM cartridge */
	if (full_path) {
		int r = CARTRIDGE_Insert(full_path);
		if (r < 0) {
			Log_print("Error inserting cartridge \"%s\": %s", full_path,
					  r == CARTRIDGE_CANT_OPEN ? "Can't open file" :
					  r == CARTRIDGE_BAD_FORMAT ? "Bad format" :
					  r == CARTRIDGE_BAD_CHECKSUM ? "Bad checksum" :
					  "Unknown error");
		}
		if (r > 0) {
#ifdef BASIC
			Log_print("Raw cartridge images not supported in BASIC version!");
#else /* BASIC */
			
#ifndef __PLUS
			printf("Selecting cart type");
            //			UI_is_active = TRUE;
            //			CARTRIDGE_type = UI_SelectCartType(r);
            //			UI_is_active = FALSE;
			CARTRIDGE_type = CARTRIDGE_5200_32;
#else /* __PLUS */
			CARTRIDGE_type = (CARTRIDGE_NONE == nCartType ? UI_SelectCartType(r) : nCartType);
#endif /* __PLUS */
			CARTRIDGE_Start();
			
#endif /* BASIC */
		}
#ifndef __PLUS
		if (CARTRIDGE_type != CARTRIDGE_NONE) {
			int for5200 = CARTRIDGE_IsFor5200(CARTRIDGE_type);
			if (for5200 && Atari800_machine_type != Atari800_MACHINE_5200) {
				Atari800_machine_type = Atari800_MACHINE_5200;
				MEMORY_ram_size = 16;
				Atari800_InitialiseMachine();
			}
			else if (!for5200 && Atari800_machine_type == Atari800_MACHINE_5200) {
				Atari800_machine_type = Atari800_MACHINE_XLXE;
				MEMORY_ram_size = 64;
				Atari800_InitialiseMachine();
			}
		}
#endif /* __PLUS */
	}
	
	//Get the game properties
	//info->data, info->size

   return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   (void)game_type;
   (void)info;
   (void)num_info;
   return false;
}

void retro_unload_game(void) 
{

}

unsigned retro_get_region(void)
{
    //stella->GameConsole->getFramerate();
    return RETRO_REGION_NTSC;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id)
{
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    return 0;
}

void retro_init()
{
    const char *dir = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
    {
        char bios_5200_rom[10] = "/5200.rom";

        strcpy(bios_filename, dir);
        strcat(bios_filename, bios_5200_rom);
        
		fprintf(stderr, "5200 BIOS path: %s\n", bios_filename);
    }
    else
    {
        fprintf(stderr, "System directory is not defined. Cannot continue ...\n");
        failed_init = true;
    }

   // Hints that we need a fairly powerful system to run this.
   unsigned level = 3;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

void retro_deinit(void)
{
    Atari800_Exit(false);
}

void retro_reset(void)
{
	Atari800_Coldstart();
}

void retro_run(void)
{
	//INPUT
    update_input();
    
    //EMULATE
    // Note: this triggers UI code and also calls the input functions above
	Atari800_Frame();
    
    //VIDEO
    /*
    //This this was 32-bit BGRA
	UBYTE *source = (UBYTE *)(Screen_atari);
	UBYTE *destination = screenBuffer;
	for (int i = 0; i < Screen_HEIGHT; i++)
    {
        for (int j = 0; j < Screen_WIDTH; j++)
        {
			UBYTE r = Colours_GetR(*source);
			UBYTE g = Colours_GetG(*source);
			UBYTE b = Colours_GetB(*source);
			*destination++ = b;
			*destination++ = g;
			*destination++ = r;
			*destination++ = 0xff;
			source++;
		}
	}
    video_cb(screenBuffer, Screen_WIDTH, Screen_HEIGHT, Screen_WIDTH * 2);*/
    
    //Convert to 16-bit XRGB
    UBYTE *source = (UBYTE *)(Screen_atari);
    for(int i = 0; i < Screen_HEIGHT; i ++)
    {
        for(int j = 0; j < Screen_WIDTH; j ++)
        {
            unsigned short color = (Colours_GetB(*source) >> 3); // B
            color |= (Colours_GetG(*source) >> 3) << 5; // G
            color |= (Colours_GetR(*source) >> 3) << 10; // R
            frame_buffer[i * Screen_WIDTH + j] = color;
            source++;
        }
    }
    video_cb(frame_buffer, Screen_WIDTH, Screen_HEIGHT, Screen_WIDTH * 2);
	
    //AUDIO
    //TODO: fix
    audio_batch_cb(NULL, NULL);
}
