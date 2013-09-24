#include "lab3d.h"
#include "adlibemu.h"

/* Size of sounds.kzp should be this in normal cases. */
// SND_FILE_LENGTH (lab3dversion==2?196423:(lab3dversion?309037:294352))

/* Initialise the game: set up most I/O and data... */

void initialize()
{
    K_INT16 i, j, k, walcounter, oclockspeed;
    K_UINT16 l;
    char *v;
    time_t tnow;
    SDL_Surface *screen;
    SDL_Surface *icon;

    SDL_AudioSpec want;
    FILE *file;
    struct stat fstats;

    int realr,realg,realb,realz,reald;
    long sndsize;

    statusbaryoffset=250;
    spriteyoffset=0;
    ingame=0;
    mixing=0;
    menuing=0;
    speed = 240;
    musicstatus=0;

    visiblescreenyoffset=0;

    soundmutex=SDL_CreateMutex();
    timermutex=SDL_CreateMutex();

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
    
    SDL_ShowCursor(0);

    fprintf(stderr,"Activating video...\n");

    icon=SDL_LoadBMP("ken.bmp");
    if (icon==NULL) {
	fprintf(stderr,"Warning: ken.bmp (icon file) not found.\n");
    }
    SDL_WM_SetIcon(icon,NULL);
    if ((screen=SDL_SetVideoMode(screenwidth, screenheight, 32, 
				 fullscreen?
				 (SDL_OPENGL|SDL_FULLSCREEN):SDL_OPENGL))==
	NULL) {
	fprintf(stderr,"True colour failed; taking whatever is available.\n");
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,16);
	if ((screen=SDL_SetVideoMode(screenwidth, screenheight, 0, 
				     fullscreen?
				     (SDL_OPENGL|SDL_FULLSCREEN):SDL_OPENGL))==
	    NULL) {
	    fprintf(stderr,"Video mode set failed.\n");
	    SDL_Quit();
	    exit(-1);
	}
    }	

    SDL_GL_GetAttribute(SDL_GL_RED_SIZE,&realr);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE,&realg);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE,&realb);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE,&realz);
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER,&reald);

    fprintf(stderr,"GL Vendor: %s\n",glGetString(GL_VENDOR));
    fprintf(stderr,"GL Renderer: %s\n",glGetString(GL_RENDERER));
    fprintf(stderr,"GL Version: %s\n",glGetString(GL_VERSION));
    //fprintf(stderr,"GL Extensions: %s\n",glGetString(GL_EXTENSIONS));

    fprintf(stderr,"GLU Version: %s\n",gluGetString(GLU_VERSION));
    //fprintf(stderr,"GLU Extensions: %s\n",gluGetString(GLU_EXTENSIONS));

    if (reald==0) {
	fprintf(stderr,"Double buffer not available.\n");
	SDL_Quit();
	exit(-1);
    }

    if (realz<16) {
	fprintf(stderr,"Warning: Depth buffer resolution too low; expect\n");
	fprintf(stderr,"graphical glitches.\n");
	SDL_Quit();
	exit(-1);
    }

    SDL_SetGamma(gammalevel,gammalevel,gammalevel);

    if (realz<24) {
	walltol=256; neardist=128;
    } else {
	walltol=32; neardist=16;
    }

    fprintf(stderr,
	    "Opened GL at %d/%d/%d (R/G/B) bits, %d bit depth buffer.\n",
	    realr,realg,realb,realz);

    /* SDL on Windows does funny things if the specified window size or
       fullscreen resolution is invalid. SDL on X fakes it seamlessly. The
       documentation doesn't mention anything about this, so I assume that this
       aspect of SDL is undefined. */

    if ((screenwidth!=screen->w)||(screenheight!=screen->h)) {
	fprintf(stderr,"Warning: screen resolution is actually %dx%d.\n",
		screen->w,screen->h);
	if ((screen->w<screenwidth)||(screen->h<screenheight)) {
	    fprintf(stderr,"Too small to pad; using full screen.\n");
	    screenwidth=screen->w;
	    screenheight=screen->h;	    
	}
	else {
	    glViewport((screen->w-screenwidth)>>1,(screen->h-screenheight)>>1,
		       screenwidth,screenheight);
	    fprintf(stderr,"Using a viewport within the screen.\n");
	}
    }

    /* Actually, this mode seems to be both more compatible and faster, so
       I think I'll leave it like this. Change this to 1 at your own risk. */

    largescreentexture=0;
							      
    if (largescreentexture) {
	/* One large 512x512 texture. */

	screenbufferwidth=screenbufferheight=512;
    } else {
	/* 6*11 matrix of 64x64 tiles with 1 pixel wide borders on shared
	   edges. */

	screenbufferwidth=374;
	screenbufferheight=746;
    }

    screenbuffer=malloc(screenbufferwidth*screenbufferheight);
    screenbuffer32=malloc(screenbufferwidth*screenbufferheight*4);

    SDL_WM_SetCaption("Ken's Labyrinth", "Ken's Labyrinth");

    linecompare(479);

    if (screenbuffer==NULL) {
	fprintf(stderr,"Insufficient memory.\n");
	SDL_Quit();
	exit(-1);
    }
    
    fprintf(stderr,"Loading tables/settings...\n");

    loadtables();
    vidmode = 1; /* Force fake 360x240 mode. */
    mute = 0;
    if ((joyx1 == joyx2) || (joyx2 == joyx3) || (joyy1 == joyy2) ||
	(joyy2 == joyy3))
	joystat = 1;

    if (joystat==0) {
        fprintf(stderr,"Opening joystick...\n");
	joystick=SDL_JoystickOpen(0);
	SDL_JoystickEventState(1);
    }

    if (joystick==NULL) joystat=1;

    time(&tnow);
    srand((unsigned int)tnow);
    if ((note = malloc(16384)) == NULL)
    {
	fprintf(stderr,"Error #1:  Memory allocation failed.\n");
	SDL_Quit();
	exit(-1);
    }

    if (musicsource==1) {
	fprintf(stderr,"Opening music output...\n");
#ifdef WIN32
	if ((i=midiOutOpen(&sequencerdevice,MIDI_MAPPER,(DWORD)(NULL),
			   (DWORD)(NULL),0))!=
	    MMSYSERR_NOERROR) {
	    fprintf(stderr,"Failed to open MIDI Mapper; error %d.\n",i);
	    SDL_Quit();
	    exit(-1);
	}
#endif
#ifdef USE_OSS
	sequencerdevice=open("/dev/sequencer", O_WRONLY, 0);
	if (sequencerdevice<0) {
	    fprintf(stderr,"Music failed opening /dev/sequencer.\n");
	    SDL_Quit();
	    exit(-1);
	}
	if (ioctl(sequencerdevice, SNDCTL_SEQ_NRMIDIS, &nrmidis) == -1) {
	    fprintf(stderr, "Can't get info about midi ports!\n");
	    SDL_Quit();
	    exit(-1);
	}
#endif
    }

    if (musicsource==2) {
	fprintf(stderr,"Opening Adlib emulation for %s music (%s output)...\n",
		musicpan?"stereo":"mono",(channels-1)?"stereo":"mono");
	adlibinit(44100,channels,2);
	adlibsetvolume(musicvolume*48);
    }

    if (speechstatus >= 2)
    {	
	if (((i = open("sounds.kzp",O_BINARY|O_RDONLY,0)) != -1)||
	    ((i = open("SOUNDS.KZP",O_BINARY|O_RDONLY,0)) != -1)) {
	    fstat(i, &fstats);
	    sndsize = (int)(fstats.st_size);
	    fprintf(stderr, "Detected %ld byte sounds.\n", sndsize);
	    close(i);
	} else sndsize=0;

	SoundFile=malloc(sndsize);

	SoundBuffer=malloc(65536*2);

	if ((SoundFile==NULL)||(SoundBuffer==NULL)) {
	    fprintf(stderr,"Insufficient memory for sound.\n");
	    SDL_Quit();
	    exit(-1);
	}

	file=fopen("sounds.kzp","rb");
	if (file==NULL) {
	    file=fopen("SOUNDS.KZP","rb");
	}
	if (file==NULL) {
	    fprintf(stderr,"Can not find sounds.kzp.\n");
	    SDL_Quit();
	    exit(-1);
	}
	if (fread(SoundFile,1,sndsize,file)!=sndsize) {
	    fprintf(stderr,"Error in sounds.kzp.\n");
	    SDL_Quit();
	    exit(-1);
	}
	fclose(file);

	SDL_LockMutex(soundmutex);
	fprintf(stderr,"Opening sound output in %s for %s sound effects...\n",
		(channels-1)?"stereo":"mono",
		soundpan?"stereo":"mono");

	want.freq=(musicsource==2)?44100:11025;
	want.format=AUDIO_S16SYS;
	want.channels=channels;
	want.samples=soundblocksize;
	want.userdata=NULL;
	want.callback=AudioCallback;	
	soundbytespertick=(channels*want.freq*2)/240;
	soundtimerbytes=0;	
	
	SDL_OpenAudio(&want,NULL);

	reset_dsp();

	SDL_UnlockMutex(soundmutex);
	SDL_PauseAudio(0);
    } else {
	if (soundtimer)
	    fprintf(stderr,"Warning: no sound, using system timer.\n");
	soundtimer=0;
    }
    fprintf(stderr,"Allocating memory...\n");
    if (((lzwbuf = malloc(12304-8200)) == NULL)||
	((lzwbuf2=malloc(8200))==NULL))
    {
	fprintf(stderr,"Error #3: Memory allocation failed.\n");
	SDL_Quit();
	exit(-1);
    }

    convwalls = numwalls;

    if ((pic = malloc((numwalls-initialwalls)<<12)) == NULL)
    {
	fprintf(stderr,
		"Error #4: This computer does not have enough memory.\n");
	SDL_Quit();
	exit(-1);
    }
    walcounter = initialwalls;
    if (convwalls > initialwalls)
    {
	v = pic;
	for(i=0;i<convwalls-initialwalls;i++)
	{
	    walseg[walcounter] = v;
	    walcounter++;
	    v += 4096;
	}
    }
    l = 0;
    for(i=0;i<240;i++)
    {
	times90[i] = l;
	l += 90;
    }
    less64inc[0] = 16384;
    for(i=1;i<64;i++)
	less64inc[i] = 16384 / i;
    for(i=0;i<256;i++)
	keystatus[i] = 0;
    for(i=0;i<SDLKEYS;i++)
	newkeystatus[i]=0;

    fprintf(stderr,"Allocating screen buffer texture...\n");
    if (largescreentexture) {
	glGenTextures(1,&screenbuffertexture);
    } else {
	glGenTextures(72,screenbuffertextures);
    }

    fprintf(stderr,"Loading intro music...\n");
    saidwelcome = 0;
    loadmusic("BEGIN");
    clockspeed = 0;
    slottime = 0;
    owecoins = 0;
    owecoinwait = 0;
    slotpos[0] = 0;
    slotpos[1] = 0;
    slotpos[2] = 0;
    clockspeed = 0;
    skilevel = 0;
    musicon();
    fprintf(stderr,"Loading intro pictures...\n");

    if (lab3dversion) {
	kgif(-1);
	k=0;
	for(i=0;i<16;i++)
	    for(j=1;j<17;j++)
	    {
		spritepalette[k++] = (opaldef[i][0]*j)/17;
		spritepalette[k++] = (opaldef[i][1]*j)/17;
		spritepalette[k++] = (opaldef[i][2]*j)/17;
	    }
	fprintf(stderr,"Loading old graphics...\n");
	loadwalls(1);
    } else {
	/* The ingame palette is stored in this GIF! */
	kgif(1);
	memcpy(spritepalette,palette,768);

	/* Show the Epic Megagames logo while loading... */

	kgif(0);
	fprintf(stderr,"Loading graphics...\n");
	loadwalls(1);

	/* Ken's Labyrinth logo. */
	if (!kgif(2))
	    kgif(1);

	fade(63);
    }  

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    SetVisibleScreenOffset(0);
    SDL_GL_SwapBuffers();

    if (moustat == 0)
	moustat = setupmouse();
    SDL_LockMutex(timermutex);
    oclockspeed = clockspeed;
    while ((keystatus[1] == 0) && (keystatus[57] == 0) && 
	   (keystatus[28] == 0) && (bstatus == 0) &&
	   (clockspeed < oclockspeed+960))
    {
	PollInputs();

	bstatus = 0;
	if (moustat == 0)
	{
	    bstatus=readmouse(NULL, NULL);
	}
	if (joystat == 0)
	{
	    bstatus|=readjoystick(NULL,NULL);
	}
	SDL_UnlockMutex(timermutex);
	SDL_Delay(10);
	SDL_LockMutex(timermutex);
    }
    SDL_UnlockMutex(timermutex);

    /* Big scrolly picture... */

    j=0;

    if (lab3dversion==0) {
	fade(0);
	j=kgif(2);
	if (j)
	    kgif(0);

	fade(63);
	l = 25200;
	i = 1;
    }

    SDL_LockMutex(timermutex);
    oclockspeed=clockspeed;

    while ((keystatus[1] == 0) && (keystatus[57] == 0) &&
	   (keystatus[28] == 0) && (bstatus == 0) &&
	   (clockspeed < ((lab3dversion|j)?3840:7680)))
    {
	if (i == 0)
	{
	    l += 90;
	    if (l >= 25200)
	    {
		i = 1;
		l = 25200;
	    }
	}
	if (i == 1)
	{
	    l -= 90;
	    if (l > 32768)
	    {
		l = 0;
		i = 0;
	    }
	}

	while(clockspeed<oclockspeed+12) {
	    SDL_UnlockMutex(timermutex);
	    SDL_Delay(10);
	    SDL_LockMutex(timermutex);
	}
	oclockspeed+=12;

	if (!(lab3dversion|j)) {
	    SDL_UnlockMutex(timermutex);
	    glClearColor(0,0,0,0);
	    glClear( GL_COLOR_BUFFER_BIT);
	    visiblescreenyoffset=(l/90)-20;
	    ShowPartialOverlay(20,20+visiblescreenyoffset,320,200,0);
	    SDL_GL_SwapBuffers();
	    SDL_LockMutex(timermutex);
	}
	PollInputs();
	bstatus = 0;
	if (moustat == 0)
	{
	    bstatus=readmouse(NULL, NULL);
	}
	if (joystat == 0)
	{
	    bstatus|=readjoystick(NULL,NULL);
	}
    }
    oclockspeed=clockspeed;
    for(i=63;i>=0;i-=4)
    {
	SDL_UnlockMutex(timermutex);
	fade(64+i);
	glClearColor(0,0,0,0);
	glClear( GL_COLOR_BUFFER_BIT);
	if (lab3dversion|j)
	    visiblescreenyoffset=0;
	else
	    visiblescreenyoffset=(l/90)-20;
	ShowPartialOverlay(20,20+visiblescreenyoffset,320,200,0);
	SDL_GL_SwapBuffers();
	SDL_LockMutex(timermutex);

	while(clockspeed<oclockspeed+4) {
	    SDL_UnlockMutex(timermutex);
	    SDL_Delay(10);
	    SDL_LockMutex(timermutex);
	}
	oclockspeed+=4;
    }
    SDL_UnlockMutex(timermutex);
    k = 0;
    for(i=0;i<16;i++)
	for(j=1;j<17;j++)
	{
	    palette[k++] = (paldef[i][0]*j)/17;
	    palette[k++] = (paldef[i][1]*j)/17;
	    palette[k++] = (paldef[i][2]*j)/17;
	}
    lastunlock = 1;
    lastshoot = 1;
    lastbarchange = 1;
    hiscorenamstat = 0;

    /* Shareware/registered check... */

    if (lab3dversion) {
	if (((i = open("boards.dat",O_BINARY|O_RDONLY,0)) != -1)||
	    ((i = open("BOARDS.DAT",O_BINARY|O_RDONLY,0)) != -1)) {
	    fstat(i, &fstats);
	    numboards = (int)(fstats.st_size>>13);
	    fprintf(stderr, "Detected %d boards.\n", numboards);
	    close(i);
	} else {
	    fprintf(stderr,"boards.dat not found.\n");
	    SDL_Quit();
	    exit(1);
	}
    } else {
	if (((i = open("boards.kzp",O_RDONLY|O_BINARY,0)) != -1)||
	    ((i = open("BOARDS.KZP",O_RDONLY|O_BINARY,0)) != -1)) {
	    readLE16(i,&boleng[0],30*4);
	    numboards = 30;
	    if ((boleng[40]|boleng[41]) == 0)
		numboards = 20;
	    if ((boleng[20]|boleng[21]) == 0)
		numboards = 10;
	    close(i);
	} else {
	    fprintf(stderr,"boards.kzp not found.\n");
	    SDL_Quit();
	    exit(1);
	}
    }
    musicoff();
}
