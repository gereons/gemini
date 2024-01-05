/* 
 * shell0.c - fÅr Turbo C 
 * setzt _shell_p auf NULL, gehîrt in jeden AUTO-Ordner
 * 30/06/89 gs
 */
#include <tos.h>

main(void)
{
	long oldssp = Super(0L);
	
	*((long *)0x4f6L) = 0L;
	Super((void *)oldssp);
	return 0;
}

