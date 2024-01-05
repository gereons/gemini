/*
 * @(#) Mupfel\MGlob.h
 * @(#) Julian F. Reschke & Stefan Eissing, 06. Februar 1991
 */
 

#ifndef __mglob__
#define __mglob__

typedef struct
{
	struct							/* Commandline/set-Switches */
	{
		unsigned export_all : 1;				/* -a */
		unsigned auto_exit : 1;					/* -e */
		unsigned no_globber : 1;				/* -f */
		unsigned locate_function_commands : 1;	/* -h */
		unsigned interactive : 1;				/* -i */
		unsigned keywords_everywhere : 1;		/* -k */
		unsigned local_block : 1;				/* -l */
		unsigned no_execution : 1;				/* -n */
		unsigned only_one_command : 1;			/* -t */
		unsigned no_empty_vars : 1;				/* -u */
		unsigned verbose : 1;					/* -v */
		unsigned print_as_exec : 1;				/* -x */
		
		unsigned dont_use_gem : 1;				/* -G */
	} shell_flags;
	
	struct
	{
		int brk;				/* reset parser: ^C */
		int kill;				/* clear line: Esc */
		int completion;			/* filename completion: Tab */
		int meta_completion;	/* display possible compl. Help */
		char *fkeys[20];		/* Belegung der Funktionstasten */
	} tty_chars;
	
	
} MGLOBAL;


extern MGLOBAL M;

void InitMGlobal (MGLOBAL *m);

#endif /* __mglob__ */