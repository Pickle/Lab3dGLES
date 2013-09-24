#ifndef WIZCAANOO_H
#define WIZCAANOO_H

#if defined(WIZ) || defined(CAANOO)

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#ifdef WIZ_TIMER
#define SDL_GetTicks    wiz_timer_read
#define SDL_Delay       wiz_timer_delay
#endif

#if defined(CAANOO)
#include <linux/input.h>
#else
#define GPH_UP                          (1<<0)
#define GPH_UP_LEFT                     (1<<1)
#define GPH_LEFT                        (1<<2)
#define GPH_DOWN_LEFT                   (1<<3)
#define GPH_DOWN                        (1<<4)
#define GPH_DOWN_RIGHT                  (1<<5)
#define GPH_RIGHT                       (1<<6)
#define GPH_UP_RIGHT                    (1<<7)
#endif
#define GPH_START                       (1<<8)          // Caanoo HOME
#define GPH_SELECT                      (1<<9)          // Cannoo HELP2
#define GPH_L                           (1<<10)
#define GPH_R                           (1<<11)
#define GPH_A                           (1<<12)
#define GPH_B                           (1<<13)
#define GPH_Y                           (1<<14)
#define GPH_X                           (1<<15)
#define GPH_VOL_UP                      (1<<16)
#define GPH_VOL_DOWN                    (1<<17)
#define GPH_PUSH                        (1<<18)         // Caanoo only
#define GPH_HELP1                       (1<<20)         // Caanoo only

#if defined(WIZ)
#define VOLUME_MIN          0
#define VOLUME_MAX          100
#define VOLUME_CHANGE_RATE  2
#define VOLUME_NOCHG        0
#define VOLUME_DOWN         1
#define VOLUME_UP           2
#define VOLUME_TIME_RATE    20
#endif

int InitWizCaanoo( void );
void CloseWizCaanoo( void );
#if defined(WIZ_TIMER)
void wiz_timer_init( void );
void wiz_timer_close( void );
unsigned long wiz_timer_read( void );
void wiz_timer_delay( unsigned long delay );
#endif
int GetButttonState( int type, int value, int button );
int GetButtonsWizCaanoo( void );
#if defined(WIZ)
void AdjustVolumeWiz( int direction );
void SplashWizCaanoo( void );
#endif

#if defined(CAANOO)
void ReadEvents( int fd );
#endif

#endif

#endif // WIZCAANOO_H
