/*
 * commands.c  -  list of internal commands
 * 23.01.91
 *
 * 10.01.91 changes for dir, vdir, ls, getopt (jr)
 * 17.01.91 file removed, smallcmd introduced
 * 18.01.91 getopt
 * 23.01.91 runopts
 * 13.02.91 find eliminiert, du neu
 * 11.05.91 wc eliminiert
 * 06.06.91 df eliminiert
 */

#include "comm.h"
#include "commands.h"
#include "ls.h"
#include "mupfel.h"

#if LANGUAGE == LNG_GERMAN
	#include "cmd_ger.h"
#elif LANGUAGE == LNG_ENGLISH
	#include "cmd_eng.h"
#endif

SCCS(commands);

typedef int command(ARGCV);


command m_getopt;	/* mgetopt.c */
command m_exit;	/* exit.c */
command m_echo;	/* echo.c */
command m_whereis;	/* whereis.c */
command m_cd;		/* curdir.c */
command m_pwd;		/* curdir.c */
command m_date;	/* date.c */
command m_cat;		/* cat.c */
command m_setenv;	/* environ.c */
command m_printenv;	/* environ.c */
command m_help;	/* help.c */
command m_fkey;	/* fkey.c */
command m_mkdir;	/* mkdir.c */
command m_rmdir;	/* rm.c */
command m_touch;	/* date.c */
command m_rm;		/* rm.c */
command m_chmod;	/* chmod.c */
command m_runopts;	/* chmod.c */
command m_cpmv;	/* cpmv.c */
command m_free;	/* free.c */
command m_df;		/* df.c */
command m_backup;	/* cpmv.c */
command m_tree;	/* tree.c */
command m_version;	/* version.c */
command m_hash;	/* hash.c */
command m_set;		/* shellvar.c */
command m_label;	/* label.c */
command m_du;		/* tree.c */
command m_pushd;	/* curdir.c */
command m_popd;	/* curdir.c */
command m_dirs;	/* curdir.c */
command m_init;	/* init.c */
command m_rename;	/* cpmv.c */
command m_timer;	/* timer.c */
command m_alias;	/* alias.c */
command m_setscrap;	/* scrap.c */
command m_blitmode;	/* blitter.c */
command m_shrink;	/* shrink.c */
command m_pause;	/* echo.c */
command m_print;	/* print.c */
command m_more;	/* more.c */
command m_noalias;	/* alias.c */
command m_cookie;	/* cookie.c */
command m_rsconf;	/* control.c */
command m_prtconf;	/* control.c */
command m_kbrate;	/* control.c */
command m_kclick;	/* control.c */
command m_format;	/* format.c */
command m_errorfile;/* errfile.c */
command m_true;		/* smallcmd.c */
command m_false;	/* smallcmd.c */
command m_basename;	/* smallcmd.c */
command m_dirname;	/* smallcmd.c */
command m_getopt;	/* smallcmd.c */
command m_test;		/* test.c */

struct cmds interncmd[] =
{
	{"alias",		FALSE,	m_alias,		OPT_alias,	HLP_alias},
	{"backup",	TRUE,	m_backup,		OPT_backup,	HLP_backup},
	{"basename",	FALSE,	m_basename,	OPT_basename,		HLP_basename},
	{"blitmode",	TRUE,	m_blitmode,	OPT_blitmode,	HLP_blitmode},
	{"cat",		TRUE,	m_cat,		OPT_cat,		HLP_cat},
	{"cd",		TRUE,	m_cd,		OPT_cd,		HLP_cd},
	{"chmod",		TRUE,	m_chmod,		OPT_chmod,	HLP_chmod},
	{"cookie",	TRUE,	m_cookie,		OPT_cookie,	HLP_cookie},
	{"cp",		TRUE,	m_cpmv,		OPT_cp,		HLP_cp},
	{"date",		TRUE,	m_date,		OPT_date,		HLP_date},
	{"dir",		FALSE,	m_ls,		OPT_dir,		HLP_dir},
	{"dirname",		FALSE,	m_dirname,	OPT_dirname,		HLP_dirname},
	{"dirs",		TRUE,	m_dirs,		OPT_dirs,		HLP_dirs},
	{"du",		TRUE,	m_du,		OPT_du,		HLP_du},
	{"echo",		FALSE,	m_echo,		OPT_echo,		HLP_echo},
	{"env",		TRUE,	m_printenv,	OPT_env,		HLP_env},
	{"errorfile",	TRUE,	m_errorfile,	OPT_errorfile,	HLP_errorfile},
	{"exit",		TRUE,	m_exit,		OPT_exit,		HLP_exit},
	{"false",		FALSE,	m_false,	OPT_false,		HLP_false},
	{"fkey",		FALSE,	m_fkey,		OPT_fkey,		HLP_fkey},
	{"format",	TRUE,	m_format,		OPT_format,	HLP_format},
	{"free",		TRUE,	m_free,		OPT_free,		HLP_free},
	{"getopt",		FALSE,	m_getopt,	OPT_getopt,		HLP_getopt},
	{"hash",		TRUE,	m_hash,		OPT_hash,		HLP_hash},
	{"help",		TRUE,	m_help,		OPT_help,		HLP_help},
	{"init",		TRUE,	m_init,		OPT_init,		HLP_init},
	{"kbrate",	TRUE,	m_kbrate,		OPT_kbrate,	HLP_kbrate},
	{"kclick",	TRUE,	m_kclick,		OPT_kclick,	HLP_kclick},
	{"label",		TRUE,	m_label,		OPT_label,	HLP_label},
	{"ls",		FALSE,	m_ls,		OPT_ls,		HLP_ls},
	{"mkdir",		TRUE,	m_mkdir,		OPT_mkdir,	HLP_mkdir},
	{"more",		TRUE,	m_more,		OPT_more,		HLP_more},
	{"mv",		TRUE,	m_cpmv,		OPT_mv,		HLP_mv},
	{"noalias",	FALSE,	m_noalias,	OPT_noalias,	HLP_noalias},
	{"pause",		FALSE,	m_pause,		OPT_pause,	HLP_pause},
	{"popd",		TRUE,	m_popd,		OPT_popd,		HLP_popd},
	{"print",		TRUE,	m_print,		OPT_print,	HLP_print},
	{"prtconf",	TRUE,	m_prtconf,	OPT_prtconf,	HLP_prtconf},
	{"pushd",		TRUE,	m_pushd,		OPT_pushd,	HLP_pushd},
	{"pwd",		TRUE,	m_pwd,		OPT_pwd,		HLP_pwd},
	{"rename",	TRUE,	m_rename,		OPT_rename,	HLP_rename},
	{"rm",		TRUE,	m_rm,		OPT_rm,		HLP_rm},
	{"rmdir",		TRUE,	m_rmdir,		OPT_rmdir,	HLP_rmdir},
	{"rsconf",	TRUE,	m_rsconf,		OPT_rsconf,	HLP_rsconf},
	{"runopts",	TRUE,	m_runopts,		OPT_runopts,	HLP_runopts},
	{"set",		TRUE,	m_set,		OPT_set,		HLP_set},
	{"setenv",	FALSE,	m_setenv,		OPT_setenv,	HLP_setenv},
	{"setscrap",	TRUE,	m_setscrap,	OPT_setscrap,	HLP_setscrap},
	{"show",		TRUE,	m_printenv,	OPT_show,		HLP_show},
	{"shrink",	TRUE,	m_shrink,		OPT_shrink,	HLP_shrink},
	{"test",		TRUE,	m_test,		OPT_test,	HLP_test},
	{"timer",		TRUE,	m_timer,		OPT_timer,	HLP_timer},
	{"touch",		TRUE,	m_touch,		OPT_touch,	HLP_touch},
	{"tree",		FALSE,	m_tree,		OPT_tree,		HLP_tree},
	{"true",		FALSE,	m_true,		OPT_true,		HLP_true},
	{"vdir",		FALSE,	m_ls,		OPT_vdir,		HLP_vdir},
	{"version",	TRUE,	m_version,	OPT_version,	HLP_version},
	{"whereis",	TRUE,	m_whereis,	OPT_whereis,	HLP_whereis},
	{"[",		TRUE,	m_test,		OPT_testk,	HLP_test},
	{":",		FALSE,	m_true,		OPT_empty,	HLP_empty},
};

long interncount = DIM(interncmd);

int MupfelCommand(MupfelFunction f, int argc, char **argv)
{
	int retcode;
	
	shellcmd = TRUE;
	switch(f)
	{
		case CP:
			argv[0] = "cp";
			retcode = m_cpmv(argc,argv);
			break;
		case MV:
			argv[0] = "mv";
			retcode = m_cpmv(argc,argv);
			break;
		case RM:
			argv[0] = "rm";
			retcode = m_rm(argc,argv);
			break;
		case LABEL:
			argv[0] = "label";
			retcode = m_label(argc,argv);
			break;
		case INIT:
			argv[0] = "init";
			retcode = m_init(argc,argv);
			break;
		case FORMAT:
			argv[0] = "format";
			retcode = m_format(argc,argv);
			break;
	}
	shellcmd = FALSE;
	return retcode;
}