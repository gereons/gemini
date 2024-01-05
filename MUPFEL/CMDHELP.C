/*
 * cmdhelp.c  -  help for internal commands
 * 03.10.89
 */

#error UNUSED MODULE AS OF 03 Oct 1989
 
#include <string.h>

#include "chario.h"
#include "cmdhelp.h"
#include "mupfel.h"
#include "stand.h"
#include "strsel.h"

SCCS(cmdhelp);

void cmdhelp(char *cmd)
{
	strlwr(cmd);
	mprint("%s ",cmd);
	STRSELECT(cmd)
	WHEN("help")
		mprint("[cmd] - print list of commands or help for cmd");
	WHEN3("exit","quit","venus")
#if STANDALONE
		mprint("[val] - terminate mupfel, return val or 0 to caller");
#else
		mprint("- reenter Venus");
#endif
	WHEN("whereis")
		mprint("cmd - locate cmd in $PATH");
	WHEN("echo")
		mprint("[args] - print args on stdout");
	WHEN("cd")
		mprint("[dir] - change directory to dir or $HOME");
	WHEN("pwd")
		mprint("[-a|drv:] - print working directory (-a: all drives)");
	WHEN("date")
		mprint("[hhmm|mmddhhmm[yy]] - display or set system date and time");
	WHEN("cat")
		mprint("[-l] files - print files on stdout");
	WHEN("ls")
		mprint("[-clusdrtfh] - list files (sorted single column)");
	WHEN("lc")
		mprint("[-clusdrtfh] - list files (sorted multicolumn)");
	WHEN("lu")
		mprint("[-clusdrtfh] - list files (unsorted single column)");
	WHEN("ll")
		mprint("[-clusdrtfh] - list files (sorted long format)");
	WHEN("setenv")
		mprint("[var [value]] - set or unset environment variable");
	WHEN2("show","env")
		mprint("var - display environment variable");
	WHEN("fkey")
		mprint("[key [str]] - display or define function keys");
	WHEN("touch")
		mprint("[-c] [-d hhmm|mmddhhmm[yy]] files - set file modification time");
	WHEN("rm")
		mprint("[-rvfi] files - remove files");
	WHEN("chmod")
		mprint("[+-][ahsw] files - set/clear attributes");
	WHEN("mkdir")
		mprint("dirs - create directories");
	WHEN("rmdir")
		mprint("dirs - remove directories (must be empty)");
	WHEN("mv")
		mprint("[-cvi] f1 f2 or mv f1..fn dir or mv dir1 dir2 - rename/move files");
	WHEN("cp")
		mprint("[-cvdrias] f1 f2 or cp f1..fn dir - copy files");
	WHEN("free")
		mprint("- display free main memory");
	WHEN("info")
		mprint("[-a|drv:] - display free disk space");
	WHEN("backup")
		mprint("[-icv] files - copy files, dest extension is .bak");
	WHEN("tree")
		mprint("[-di] [-f filespec] [-p path] - display file hierarchy");
	WHEN("version")
		mprint("[-amgdth] - display version numbers");
	WHEN("hash")
		mprint("[-r] - display / remove hash table");
	WHEN("wc")
		mprint("[-lcw] files - count lines, words and chars");
	WHEN("file")
		mprint("files - guess type of files");
	WHEN("set")
		mprint("[var [value]] - show, set or unset internal variables");
	WHEN("label")
		mprint("[drive [label]] - show or change label on drive");
	WHEN("du")
		mprint("[dirnames...] - show disk usage for directories");
	WHEN("pushd")
		mprint("[dir] - save dir or . on directory stack");
	WHEN("popd")
		mprint("- change dir to last pushed directory");
	WHEN("dirs")
		mprint("- display directory stack");
	WHEN("init")
		mprint("[-y][-s 1|2] drv - fast disk erase");
	WHEN("rename")
		mprint("[-icv] ext files - bulk file rename");
	WHEN("timer")
		mprint("[-s] - start/display timer");
	WHEN("alias")
		mprint("[name [replacement]] - show, set or unset alias");
	WHEN("setscrap")
		mprint("[-fq] [dir] - show or set AES scrap path");
	WHEN("blitmode")
		mprint("[on|off] - show or set blitter status"); 
	WHEN("shrink")
		mprint("[-b val][-t val][-f][-i] - shrink/free memory");
	WHEN("pause")
		mprint("[args] - echo args if any, wait for keypress");
	DEFAULT
		mprint("Sorry, no help text for %s",cmd);
	ENDSEL
	crlf();
}
