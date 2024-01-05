/*
	@(#)Mupfel/ls.c
	@(#)Julian F. Reschke, 10. Februar 1991
*/

/* Fsfirst mit normalisiertem Argument */

int normalized_fsfirst (char *filename, int attr);

/* Test auf . oder .. */

int is_not_dot_or_dotdot (char *name);

/* True fÅr drv:\ */

int might_be_dir (char *filename);

int m_ls (int argc, char **argv);


