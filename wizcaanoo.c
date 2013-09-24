#if defined(WIZ) || defined(CAANOO)

#include "SDL_image.h"
#include "SDL_keysym.h"
#include "wizcaanoo.h"
#include "lab3d.h"

int stickfd=0;
int gpiofd=0;
int oldgpkey;
int ticksup, ticksdown;
int volume;
int volume_direction;
int weapon=0;

int InitWizCaanoo( void )
{
#if defined(CAANOO)
    stickfd = open("/dev/input/event1", O_RDONLY|O_NONBLOCK);
    if(stickfd < 0)
    {
        printf( "GPIO OPEN FAIL\n");
        return 1;
    }
    else
    {
        printf("Wiz/Caanoo stick loaded\n");
    }  
#endif
    gpiofd = open("/dev/GPIO", O_RDWR | O_NDELAY );
    if(gpiofd < 0)
    {
        printf( "GPIO OPEN FAIL\n");
        return 1;
    }
    else
    {
        printf("Wiz/Caanoo buttons loaded\n");
    }

#ifdef WIZ_TIMER
    wiz_timer_init();
#endif

    return 0;
}

void CloseWizCaanoo( void )
{
    close(gpiofd);  
#if defined(CAANOO)
    close(stickfd);
#endif  
#ifdef WIZ_TIMER
    wiz_timer_close();
#endif
}

#ifdef WIZ_TIMER

#define TIMER_BASE3 0x1980
#define TIMER_REG(x) memregs32[(TIMER_BASE3 + x) >> 2]

static          unsigned long   wiz_dev_mem;
static volatile unsigned int    *memregs32;

void wiz_timer_init( void )
{
    /* open /dev/mem to access registers */
    wiz_dev_mem = open("/dev/mem", O_RDWR);
    if(wiz_dev_mem < 0) {
        fprintf(stderr, "WIZ: Unable to open /dev/mem");
    }

    /* get access to the registers */
    memregs32 = (volatile unsigned int *)mmap(0, 0x20000, PROT_READ|PROT_WRITE, MAP_SHARED, wiz_dev_mem, 0xC0000000);
    if(memregs32 == (volatile unsigned int *)0xFFFFFFFF) {
        fprintf(stderr, "WIZ: Could not mmap hardware registers\n");
    }

    TIMER_REG(0x44) = 0x922;
    TIMER_REG(0x40) = 0x0c;
    TIMER_REG(0x08) = 0x6b;
}

void wiz_timer_close( void )
{
    TIMER_REG(0x40) = 0x0c;
    TIMER_REG(0x08) = 0x23;
    TIMER_REG(0x00) = 0;
    TIMER_REG(0x40) = 0;
    TIMER_REG(0x44) = 0;

    munmap((void *)memregs32, 0x20000);
    close(wiz_dev_mem);

    printf( "WIZ: hardware timer closed\n" );
}

unsigned long wiz_timer_read( void )
{
    unsigned int microsec;

    TIMER_REG(0x08) = 0x4b;  /* run timer, latch value */
    microsec = TIMER_REG(0);
    return (microsec/1000);
}

void wiz_timer_delay( unsigned long delay )
{
    unsigned int start;

    start = wiz_timer_read();
    while(wiz_timer_read()-start < delay) {}
}

#endif

#define RELEASED    0
#define PRESSED     1
int GetButttonState( int type, int value, int button )
{
    int result = 0;

    if (type == PRESSED)    // released to pressed
    {
        if ( ((oldgpkey & button) <= 0) && ((value & button) >= 1) )
        {
            result = 1;
        }
        else result = 0;
    }
    else                    // pressed to released
    {
        if ( ((oldgpkey & button) >= 1) && ((value & button) <= 0) )
        {
            result = 1;
        }
        else result = 0;
    }

    return result;
}

int GetButtonsWizCaanoo( void )
{
    int gpkey;
    int key = 0;

    read(gpiofd, &gpkey, 4);

#if defined(CAANOO)  
    ReadEvents( stickfd );
#else  
    // Use diagonals
    if ((gpkey&GPH_UP_LEFT) && !((gpkey&GPH_UP) || (gpkey&GPH_LEFT))) {
        gpkey=gpkey|GPH_UP;
        gpkey=gpkey|GPH_LEFT;
    }
    if ((gpkey&GPH_UP_RIGHT) && !((gpkey&GPH_UP) || (gpkey&GPH_RIGHT))) {
        gpkey=gpkey|GPH_UP;
        gpkey=gpkey|GPH_RIGHT;
    }
    if ((gpkey&GPH_DOWN_LEFT) && !((gpkey&GPH_DOWN) || (gpkey&GPH_LEFT))) {
        gpkey=gpkey|GPH_DOWN;
        gpkey=gpkey|GPH_LEFT;
    }
    if ((gpkey&GPH_DOWN_RIGHT) && !((gpkey&GPH_DOWN) || (gpkey&GPH_RIGHT))) {
        gpkey=gpkey|GPH_DOWN;
        gpkey=gpkey|GPH_RIGHT;
    }

    // UP
    if(       GetButttonState( PRESSED,  gpkey, GPH_UP ) )      { key = SDLK_u;  newkeystatus[SDLK_UP] = keystatus[PCkey[SDLK_UP]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_UP ) )      { key = 0;       newkeystatus[SDLK_UP] = keystatus[PCkey[SDLK_UP]]=0; }
    // DOWN
    if(       GetButttonState( PRESSED,  gpkey, GPH_DOWN ) )    { key = SDLK_d;  newkeystatus[SDLK_DOWN] = keystatus[PCkey[SDLK_DOWN]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_DOWN ) )    { key = 0;       newkeystatus[SDLK_DOWN] = keystatus[PCkey[SDLK_DOWN]]=0; }
    // LEFT
    if(       GetButttonState( PRESSED,  gpkey, GPH_LEFT ) )    { key = SDLK_l;  newkeystatus[SDLK_LEFT] = keystatus[PCkey[SDLK_LEFT]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_LEFT ) )    { key = 0;       newkeystatus[SDLK_LEFT] = keystatus[PCkey[SDLK_LEFT]]=0; }
    // RIGHT
    if(       GetButttonState( PRESSED,  gpkey, GPH_RIGHT ) )   { key = SDLK_r;  newkeystatus[SDLK_RIGHT] = keystatus[PCkey[SDLK_RIGHT]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_RIGHT ) )   { key = 0;       newkeystatus[SDLK_RIGHT] = keystatus[PCkey[SDLK_RIGHT]]=0; }
#endif
#if defined(CAANOO)  
    // ESC
    if(       GetButttonState( PRESSED,  gpkey, GPH_HELP1 ) )   { key = SDLK_ESCAPE; newkeystatus[SDLK_ESCAPE] = keystatus[PCkey[SDLK_ESCAPE]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_HELP1 ) )   { key = 0;           newkeystatus[SDLK_ESCAPE] = keystatus[PCkey[SDLK_ESCAPE]]=0; }

    // Enter
    if(       GetButttonState( PRESSED,  gpkey, GPH_PUSH ) )  { key = SDLK_RETURN; newkeystatus[SDLK_RETURN] = keystatus[PCkey[SDLK_RETURN]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_PUSH ) )  { key = 0;           newkeystatus[SDLK_RETURN] = keystatus[PCkey[SDLK_RETURN]]=0; }
#endif
    // ESC
    if(       GetButttonState( PRESSED,  gpkey, GPH_START ) )   { key = SDLK_ESCAPE; newkeystatus[SDLK_ESCAPE] = keystatus[PCkey[SDLK_ESCAPE]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_START ) )   { key = 0;           newkeystatus[SDLK_ESCAPE] = keystatus[PCkey[SDLK_ESCAPE]]=0; }

    // Enter
    if(       GetButttonState( PRESSED,  gpkey, GPH_SELECT ) )  { key = SDLK_RETURN; newkeystatus[SDLK_RETURN] = keystatus[PCkey[SDLK_RETURN]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_SELECT ) )  { key = 0;           newkeystatus[SDLK_RETURN] = keystatus[PCkey[SDLK_RETURN]]=0; }

    // Page Down
    if(       GetButttonState( PRESSED,  gpkey, GPH_X ) )      { key = SDLK_x; newkeystatus[SDLK_PAGEUP] = keystatus[PCkey[SDLK_PAGEUP]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_X ) )      { key = 0;      newkeystatus[SDLK_PAGEUP] = keystatus[PCkey[SDLK_PAGEUP]]=0; }

    // Page Up
    if(       GetButttonState( PRESSED,  gpkey, GPH_Y ) )      { key = SDLK_y; newkeystatus[SDLK_PAGEDOWN] = keystatus[PCkey[SDLK_PAGEDOWN]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_Y ) )      { key = 0;      newkeystatus[SDLK_PAGEDOWN] = keystatus[PCkey[SDLK_PAGEDOWN]]=0; }

    // Home
    if(       GetButttonState( PRESSED,  gpkey, GPH_A ) )      { key = SDLK_a; newkeystatus[SDLK_HOME] = keystatus[PCkey[SDLK_HOME]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_A ) )      { key = 0;      newkeystatus[SDLK_HOME] = keystatus[PCkey[SDLK_HOME]]=0; }

    // End
    if(       GetButttonState( PRESSED,  gpkey, GPH_B ) )      { key = SDLK_b; newkeystatus[SDLK_END] = keystatus[PCkey[SDLK_END]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_B ) )      { key = 0;      newkeystatus[SDLK_END] = keystatus[PCkey[SDLK_END]]=0; }

    // L
    if(       GetButttonState( PRESSED,  gpkey, GPH_L ) )      { key = SDLK_l; newkeystatus[SDLK_LCTRL] = keystatus[PCkey[SDLK_LCTRL]]=1; }
    else if ( GetButttonState( RELEASED, gpkey, GPH_L ) )      { key = 0;      newkeystatus[SDLK_LCTRL] = keystatus[PCkey[SDLK_LCTRL]]=0; }

    // R
    if ( GetButttonState( PRESSED, gpkey, GPH_R ) )
    {
        key = SDLK_r;

        weapon++;
        if (weapon<0 || weapon>3)
            weapon = 1;

        switch( weapon )
        {
            case 1:
            newkeystatus[SDLK_1] = keystatus[PCkey[SDLK_1]]=1;
            break;
            case 2:
            newkeystatus[SDLK_2] = keystatus[PCkey[SDLK_2]]=1;
            break;
            case 3:
            newkeystatus[SDLK_3] = keystatus[PCkey[SDLK_3]]=1;
            break;
        }

        printf( "Weapon Change: %i\n", weapon );
    }
    else if ( GetButttonState( RELEASED, gpkey, GPH_R ) )
    {
        key = 0;

        newkeystatus[SDLK_1] = keystatus[PCkey[SDLK_1]]=0;
        newkeystatus[SDLK_2] = keystatus[PCkey[SDLK_2]]=0;
        newkeystatus[SDLK_3] = keystatus[PCkey[SDLK_3]]=0;
    }
#if defined(WIZ)
    // Volume Up
    if( (((oldgpkey & GPH_VOL_UP) == 0) && ((gpkey & GPH_VOL_UP)> 0)) ||
	(((oldgpkey & GPH_VOL_UP)  > 0) && ((gpkey & GPH_VOL_UP)> 0) && ((SDL_GetTicks() - ticksup) > VOLUME_TIME_RATE )) )
    {
        ticksup = SDL_GetTicks();
        AdjustVolumeWiz(VOLUME_UP);
    }
    // Volume Down
    if( (((oldgpkey & GPH_VOL_DOWN) == 0) && ((gpkey & GPH_VOL_DOWN)> 0)) ||
	(((oldgpkey & GPH_VOL_DOWN)  > 0) && ((gpkey & GPH_VOL_DOWN)> 0) && ((SDL_GetTicks() - ticksdown) > VOLUME_TIME_RATE )) )
    {
        ticksdown = SDL_GetTicks();
        AdjustVolumeWiz(VOLUME_DOWN);
    }
#endif
    oldgpkey=gpkey;

    return key;
}

#if defined(WIZ)
void AdjustVolumeWiz( int direction )
{
	if( direction != VOLUME_NOCHG )
	{
		if( volume <= 10 )
		{
			if( direction == VOLUME_UP )   volume += VOLUME_CHANGE_RATE/2;
			if( direction == VOLUME_DOWN ) volume -= VOLUME_CHANGE_RATE/2;
		}
		else
		{
			if( direction == VOLUME_UP )   volume += VOLUME_CHANGE_RATE;
			if( direction == VOLUME_DOWN ) volume -= VOLUME_CHANGE_RATE;
		}

		if( volume < VOLUME_MIN ) volume = VOLUME_MIN;
		if( volume > VOLUME_MAX ) volume = VOLUME_MAX;

		printf( "Volume Change: %i\n", volume );

		unsigned long soundDev = open("/dev/mixer", O_RDWR);
		if(soundDev)
		{
			int vol = ((volume << 8) | volume);
			ioctl(soundDev, SOUND_MIXER_WRITE_PCM, &vol);
			close(soundDev);
		}
	}
}

void SplashWizCaanoo( void )
{
    SDL_Surface* screen = NULL;
    SDL_Surface* splash = NULL;

    /* Initialize the SDL library and the video subsystem */
    if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
	fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
	exit(1);
    }

    screen = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
    if ( screen == NULL ) {
	fprintf(stderr, "Couldn't set 320x240x16 video mode: %s\n", SDL_GetError());
	exit(1);
    }
    SDL_ShowCursor(SDL_DISABLE);

    splash = IMG_Load( "splash1.png" );
    if ( screen == NULL ) {
	fprintf(stderr, "Couldn't load splash image: %s\n", SDL_GetError());
	exit(1);
    }

    if (SDL_BlitSurface(splash, NULL, screen, NULL) < 0)
	fprintf(stderr, "BlitSurface error: %s\n", SDL_GetError());

    SDL_UpdateRect(screen, 0, 0, splash->w, splash->h);

    SDL_Delay(2000);

    SDL_FreeSurface(splash);
    SDL_Quit();
}
#endif

#if defined(CAANOO)
#define DEAD_ZONE   64
#define ZERO_POINT 128

void ReadEvents( int fd )
{
    int key;
    struct input_event event[16];
    int rbytes, i;
  
    rbytes = read(fd, &event, sizeof(event));
    if (rbytes <= 0)
        return;
    for (i=0; i<rbytes/sizeof(struct input_event); i++) 
    {
        switch (event[i].type) 
        {
            case EV_ABS: 
                switch (event[i].code) 
                {
                    case ABS_X:
                        if (event[i].value>ZERO_POINT+DEAD_ZONE) {
                            key = SDLK_r;  newkeystatus[SDLK_RIGHT] = keystatus[PCkey[SDLK_RIGHT]]=1;
                        } else {
                            key = 0;       newkeystatus[SDLK_RIGHT] = keystatus[PCkey[SDLK_RIGHT]]=0;
                        }
                        
                        if (event[i].value<ZERO_POINT-DEAD_ZONE) {
                            key = SDLK_l;  newkeystatus[SDLK_LEFT] = keystatus[PCkey[SDLK_LEFT]]=1;
                        } else {
                            key = 0;       newkeystatus[SDLK_LEFT] = keystatus[PCkey[SDLK_LEFT]]=0;
                        }
                                                   
                        printf("x = 0x%x\n", event[i].value);
                        break;
                    case ABS_Y:
                        if (event[i].value<ZERO_POINT-DEAD_ZONE) {
                            key = SDLK_u;  newkeystatus[SDLK_UP] = keystatus[PCkey[SDLK_UP]]=1;   
                        } else {
                            key = 0;       newkeystatus[SDLK_UP] = keystatus[PCkey[SDLK_UP]]=0;
                        }
                        
                        if (event[i].value>ZERO_POINT+DEAD_ZONE) {
                            key = SDLK_d;  newkeystatus[SDLK_DOWN] = keystatus[PCkey[SDLK_DOWN]]=1;
                        } else {
                            key = 0;       newkeystatus[SDLK_DOWN] = keystatus[PCkey[SDLK_DOWN]]=0;
                        }
                        
                        printf("y = 0x%x\n", event[i].value);
                        break;
                    default   :
                        printf("type = 0x%x, code = 0x%x\n", event[i].type, event[i].code);
                }
                break;
            default:
                if (event[i].type)
                    printf("type = 0x%x, code = 0x%x\n", event[i].type, event[i].code);
                break;
        }
    }
}
#endif

#endif
