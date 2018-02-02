#ifndef _ERR_
#define _ERR_

/* wypisuje informacje o bĹÄdnym zakoĹczeniu funkcji systemowej 
i koĹczy dziaĹanie */
extern void syserr(const char *fmt, ...);

/* wypisuje informacje o bĹÄdzie i koĹczy dziaĹanie */
extern void fatal(const char *fmt, ...);

#endif
