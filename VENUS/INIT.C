/*
 * @(#) Gemini\Init.c
 * @(#) Stefan Eissing, 22. Mai 1991
 *
 * description: all the Initialisation for the Desktop
 */

#include <string.h>
#include <ctype.h>
#include <vdi.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "init.h"
#include "venus.h"
#include "util.h"
#include "fileutil.h"
#include "select.h"
#include "redraw.h"
#include "pexec.h"
#include "iconinst.h"
#include "venuserr.h"
#include "applmana.h"
#include "gemtrees.h"
#include "filedraw.h"
#include "stand.h"
#if MERGED
#include "mvwindow.h"
#endif

store_sccs_id(init);


/* externals
*/
extern WindInfo wnull;
extern ShowInfo show;
extern DeskInfo NewDesk;
extern word deskx,desky,deskw,deskh;
extern word apid,phys_handle;
extern word wchar,hchar,wbox,hbox;
extern word handle,work_in[11],work_out[57],pxy[128];
extern OBJECT *pmenu,*pcopyinfo,*pstdbox,*pshowbox;
extern OBJECT *pwildbox,*prulebox, *pruleedit;
extern OBJECT *pinstbox,*prenamebox,*pcopybox;
extern OBJECT *pnamebox,*perasebox,*pfoldbox;
extern OBJECT *pfileinfo,*pfoldinfo,*pdrivinfo;
extern OBJECT *papplbox,*pttpbox,*pchangebox;
extern OBJECT *pshredbox,*ptrashbox,*pscrapbox;
extern OBJECT *piconfile,*pfrmtbox, *pconselec;
extern OBJECT *pspecinfo,*poptiobox,*pdivbox;
extern OBJECT *pinitbox,*pfontbox, *pnewdesk;
extern OBJECT *pweditbox,*popendbox, *pfmtbox;
extern OBJECT *pappledit, *pshortcut, *pcolorbox;
extern OBJECT *ptotalinf;

extern char *sorttypstring, *sorticonstring;
extern char version[],bootpath[];

extern FONTWORK filework;

/* internal texts
 */
#define NlsLocalSection "G.init"
enum NlsLocalText{
T_ABOUT,		/*  Åber Gemini... */
T_ERASE,		/*Diese Operation lîscht sÑmtliche Daten auf 
der Diskette in Laufwerk %c:. Wollen Sie fortfahren?*/
};

#define FIXTREE		1


/* îffne virtuelle Workstation
*/
word open_vwork(void)
{
	word dummy;
	
	handle = phys_handle = graf_handle(&wchar, &hchar, &wbox, &hbox);

	v_opnvwk(work_in, &handle, work_out);
	if (handle <= 0)
		return FALSE;

	filework.handle = handle;
	filework.loaded = FALSE;
	filework.sysfonts = work_out[10];
	filework.addfonts = 0;
	filework.list = NULL;
	
	wind_get(0, WF_WORKXYWH, &deskx, &desky, &deskw, &deskh);

	pxy[0]=deskx; 
	pxy[1]=desky;
	pxy[2]=deskx + deskw - 1;
	pxy[3]=desky + deskh - 1;
	vst_alignment(handle, 0, 5, &dummy, &dummy);
	vs_clip(handle, TRUE, pxy);
	v_show_c(handle, 0);
	return TRUE;
}

static void patchBackground(void)
{
	if (work_out[13] < 3)	/* monochrom */
	{
		pnewdesk[0].ob_spec.obspec.fillpattern = IP_4PATT;
	}
	else
	{
		pnewdesk[0].ob_spec.obspec.fillpattern = IP_SOLID;
	}
}

static void alignMenu(void)
{
	word diff;
	
#if MERGED
		pmenu[MNBAR].ob_width += wchar;
		pmenu[DESK].ob_spec.free_string = " GEMINI";
		pmenu[DESK].ob_width += wchar;
		pmenu[FILES].ob_x += wchar;
		pmenu[MNFILE].ob_x += wchar;
		pmenu[SHOW].ob_x += wchar;
		pmenu[MNSHOW].ob_x += wchar;
		pmenu[OPTIONS].ob_x += wchar;
		pmenu[MNOPTION].ob_x += wchar;
		pmenu[DESKINFO].ob_spec.free_string = (char *)NlsStr(T_ABOUT);
#endif

	MenuTune(pmenu, TRUE);

	diff = deskw - (pmenu[MNOPTION].ob_x + pmenu[MNOPTION].ob_width);
	if (diff < 0)
	{
		pmenu[MNOPTION].ob_x += (diff - 1);
	}
}

static int validateZ (OBJECT *t, int ob, int *chr, int *shift, int idx)
{
	static char legals[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"!0123456789_#$%^&()+-=~;,<>|[]{}"
		"\'\"\b\x1B\x7F";
	/* Hochkommata, AnfÅhrungszeichen und Klammeraffe entfernt!
	 * se 19.01.91
	 */
	(void) t, (void) ob, (void) shift, (void) idx;
	
	*chr = toupper (*chr);
	return (NULL != strchr (legals, *chr));
}

static VALFUN validFuns[] = {validateZ};

/* get Addresses from Resourcefile
*/
word getTrees(void)
{
	OBSPEC *pob;
	word ok;

	ok = rsrc_gaddr(R_TREE,MENU,&pmenu);
	if(ok)
		ok = rsrc_gaddr(R_TREE,COPYINFO,&pcopyinfo);
	if(ok)
		ok = rsrc_gaddr(R_TREE,NEWDESK,&pnewdesk);
	if(ok)
		ok = rsrc_gaddr(R_TREE,STDBOX,&pstdbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,SHOWBOX,&pshowbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,WILDBOX,&pwildbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,RULEBOX,&prulebox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,INSTBOX,&pinstbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,RENAMBOX,&prenamebox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,COPYBOX,&pcopybox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,NAMEBOX,&pnamebox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,ERASEBOX,&perasebox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,FOLDBOX,&pfoldbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,FOLDINFO,&pfoldinfo);
	if(ok)
		ok = rsrc_gaddr(R_TREE,FILEINFO,&pfileinfo);
	if(ok)
		ok = rsrc_gaddr(R_TREE,DRIVINFO,&pdrivinfo);
	if(ok)
		ok = rsrc_gaddr(R_TREE,APPLBOX,&papplbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,TTPBOX,&pttpbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,CHANGBOX,&pchangebox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,SHREDBOX,&pshredbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,TRASHBOX,&ptrashbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,SCRAPBOX,&pscrapbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,ICONFILE,&piconfile);
	if(ok)
		ok = rsrc_gaddr(R_TREE,SPECINFO,&pspecinfo);
	if(ok)
		ok = rsrc_gaddr(R_TREE,OPTIOBOX,&poptiobox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,FONTBOX,&pfontbox);
	if (ok)
		ok = rsrc_gaddr(R_TREE,WEDIT,&pweditbox);
	if (ok)
		ok = rsrc_gaddr(R_TREE,OPENDBOX,&popendbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,DIVBOX,&pdivbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,INITBOX,&pinitbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE,CONSELEC,&pconselec);
	if(ok)
		ok = rsrc_gaddr(R_TREE, RULEEDIT, &pruleedit);
	if(ok)
		ok = rsrc_gaddr(R_TREE, APPLEDIT, &pappledit);
	if(ok)
		ok = rsrc_gaddr(R_TREE, SHORTCUT, &pshortcut);
	if(ok)
		ok = rsrc_gaddr(R_TREE, COLORBOX, &pcolorbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE, FMTBOX, &pfmtbox);
	if(ok)
		ok = rsrc_gaddr(R_TREE, TOTALINF, &ptotalinf);

	if (ok)
	{
		char **cp;
		
		ok = rsrc_gaddr(R_FRSTR, SORTICON, &cp);
		sorticonstring = *cp;

		if (ok)
		{
			ok = rsrc_gaddr(R_FRSTR, SORTTYP, &cp);
			sorttypstring = *cp;
		}
	}


	if(ok)
	{
		patchBackground();
#if FIXTREE
		FixTree(pcopyinfo);
		FixTree(pshowbox);
		FixTree(pwildbox);
		FixTree(pinstbox);
		FixTree(prulebox);
		FixTree(prenamebox);
		FixTree(pcopybox);
		FixTree(pnamebox);
		FixTree(perasebox);
		FixTree(pfoldbox);
		FixTree(pfoldinfo);
		FixTree(pfileinfo);
		FixTree(pdrivinfo);
		FixTree(papplbox);
		FixTree(pttpbox);
		FixTree(pchangebox);
		FixTree(pshredbox);
		FixTree(ptrashbox);
		FixTree(pscrapbox);
		FixTree(piconfile);
		FixTree(pspecinfo);
		FixTree(poptiobox);
		FixTree(pfontbox);
		FixTree(pweditbox);
		FixTree(popendbox);
		FixTree(pdivbox);
		FixTree(pinitbox);
		FixTree(pconselec);
		FixTree(pruleedit);
		FixTree(pappledit);
		FixTree(pshortcut);
		FixTree(pcolorbox);
		FixTree(pfmtbox);
		FixTree(ptotalinf);
#endif /* FIXTREE */

		/* Setze Funktion, um Templates fÅr Filenamen zu scannen
		 */
		FormSetValidator ("Z", validFuns);
		
		alignMenu();
		
		pob = ObjcGetObspec(pcopyinfo, CPTITLE);
		pob->free_string = version;
											/* version string */

		show.fsize = (pshowbox[SOPTSIZE].ob_state & SELECTED);
		show.fdate = (pshowbox[SOPTDATE].ob_state & SELECTED);
		show.ftime = (pshowbox[SOPTTIME].ob_state & SELECTED);
		show.normicon = TRUE;
		show.showtext = TRUE;
		menu_icheck(pmenu,BYTEXT,TRUE);
		show.sortentry = SORTNAME;
		show.m_cols = 80;
		show.m_rows = 24;
		show.m_inv = 0;
		show.m_font = 1;
		show.m_fsize = 6;
		show.m_wx = show.m_wy = 100;
		
		NewDesk.snapx = NewDesk.snapy = 2;

		menu_icheck(pmenu,show.sortentry,TRUE);
#if STANDALONE
		setDisabled(pmenu,PCONSOLE,TRUE);
#endif
		checkMupfel();
	}
	return ok;
}

void initWindows(void)
{
	wnull.handle = 0;
	wnull.nextwind = NULL;
	wnull.kind = WK_DESK;
	wnull.workx = deskx; wnull.worky = desky;
	wnull.workw = deskw; wnull.workh = deskh;
	wnull.tree = pnewdesk;
	wnull.objanz = countObjects(wnull.tree,0);
	wnull.tree[0].ob_x = deskx;
	wnull.tree[0].ob_y = desky;
	wnull.tree[0].ob_width = deskw;
	wnull.tree[0].ob_height = deskh;
	pstdbox[0].ob_x = deskx;
	pstdbox[0].ob_y = desky;
	pstdbox[0].ob_width = deskw;
	pstdbox[0].ob_height = deskh;
}

/* setze den Desktophintergrund 
*/
void initNewDesk(void)
{
	copyNewDesk(pnewdesk);
	wnull.tree = NewDesk.tree;
	wind_set(0,WF_NEWDESK,wnull.tree,0);
}

/*
 * setze Status der Parameterboxen im Init-Dialog
 */
static void setState(word state, word redraw)
{
	setDisabled(pinitbox, INITONE, state);
	setDisabled(pinitbox, INITTWO, state);
	setDisabled(pinitbox, INITNINE, state);
	setDisabled(pinitbox, INITTEN, state);
	if (redraw)
	{
		fulldraw(pinitbox, INITONE);
		fulldraw(pinitbox, INITTWO);
		fulldraw(pinitbox, INITNINE);
		fulldraw(pinitbox, INITTEN);
	}
}

void showProgress(int promille)
{
	word lastw, neww, x, y;
	
	neww = (int)((pfmtbox[FMTBACK].ob_width * (long)promille) / 1000L);
	lastw = pfmtbox[FMTFORE].ob_width;

	if (neww != lastw)
	{
		pfmtbox[FMTFORE].ob_width = neww;
		ObjcOffset(pfmtbox, FMTFORE, &x, &y);
		ObjcDraw(pfmtbox, FMTFORE, 0, 
					x + lastw - 1, y,
					neww - lastw + 2, pfmtbox[FMTFORE].ob_height);
	}
}

static int callFormat(int argc, char **argv)
{
#if MERGED
	return MupfelCommand(strcmp(argv[0], "format")? INIT : FORMAT,
				argc, argv);
#else
	char command[128] = "";
	
	for (i = 0; i < argc; ++i)
	{
		strcat(command, argv[i]);
		strcat(command, " ");
	}
	
	return system(command);
#endif
}

/*
 * void initDisk(void)
 * initialize Disk (wipe cleeeeaaaan!)
 */
void initDisk(void)
{
	DIALINFO d;
	WindInfo *wp;
	IconInfo *pii;
	word objnr, retcode, done;
	char drive[4];
	char label[MAX_FILENAME_LEN];
	word format, update;
	char *margv[6];
	char sidearg[6], sectorarg[6], labelarg[16], drivearg[2];
	int margc;
	
	if((!getOnlySelected(&wp,&objnr)) || (wp->kind != WK_DESK))
		return;

	pii = getIconInfo(&wp->tree[objnr]);
	if((pii->type != DI_FILESYSTEM) || (pii->path[0] > 'B'))
		return;
		
	drive[0] = pii->path[0];
	drive[1] = ':';
	drive[2] = '\0';
	update = FALSE;

	label[0] = '\0';
	pinitbox[INITNAME].ob_spec.tedinfo->te_ptext = label;
	
	setState(isSelected(pinitbox, INITKEEP), FALSE);
	setDisabled(pinitbox, INITESC, TRUE);
	
	DialCenter(pinitbox);
	DialStart(pinitbox,&d);
	DialDraw(&d);
	
	done = FALSE;
	do
	{
		retcode = DialDo(&d,0) & 0x7FFF;
		margc = 0;

		switch (retcode)
		{
			case INITOK:
				GrafMouse(HOURGLASS,NULL);

				setDisabled(pinitbox, INITESC, FALSE);
				fulldraw(pinitbox, INITESC);
				
				format = isSelected(pinitbox, INITFORM);
				
				margv[margc++] = format? "format" : "init";
				margv[margc++] = "-y";
				
				if (!isSelected(pinitbox, INITKEEP))
				{
					sprintf(sidearg, "-s%s",
						isSelected(pinitbox, INITTWO)? "2" : "1");
					margv[margc++] = sidearg;
					sprintf(sectorarg, "-c%s",
						isSelected(pinitbox, INITTEN)? "10" : "9");
					margv[margc++] = sectorarg;
				}
				
				if (strlen(label))
				{
					makeFullName(label);
					sprintf(labelarg, "-l%s", label);
					margv[margc++] = labelarg;
				}
				
				sprintf(drivearg, "%c", drive[0]);
				margv[margc++] = drivearg;
		
				if (venusChoice(NlsStr(T_ERASE), drive[0]))
				{
					word error;
#if MERGED
					DIALINFO FMT;

					if (format)
					{
						
						CommInfo.fun.fmt_progress = showProgress;
						pfmtbox[FMTFORE].ob_width = 0;
						DialCenter(pfmtbox);
						DialStart(pfmtbox, &FMT);
						DialDraw(&FMT);
					}
#endif
					if ((error = callFormat(margc, margv)) != 0)
						sysError(error);
					else
						done = TRUE;
#if MERGED
					if (format)
					{
						DialEnd(&FMT);
						CommInfo.fun.fmt_progress = NULL;
					}
#endif					
					update = TRUE;
				}
				
				if (!done)
				{
					GrafMouse(ARROW, NULL);
					setSelected(pinitbox, INITOK, FALSE);
					fulldraw(pinitbox, INITOK);
				}
				
				break;
			case INITFORM:
			case INITSOFT:
				if (isDisabled(pinitbox, INITTWO))
					setState(FALSE,TRUE);
				break;
			case INITKEEP:
				if (!isDisabled(pinitbox, INITTWO))
					setState(TRUE,TRUE);
				break;
			default:
				done = TRUE;
				break;
		}
	}
	while (!done);

	setSelected(pinitbox, retcode, FALSE);
	DialEnd(&d);
	
	if (update)
	{
		strcat(drive,"\\");
		pathUpdate(drive,"");
	}
	GrafMouse(ARROW,NULL);
}


word mupVenus(char *autoexec)
{
	word retcode,startmode;
	char cmdline[MAXLEN];
	
	strcpy(cmdline,bootpath);
	addFileName(cmdline,autoexec);

	if(fileExists(cmdline))
	{
		getStartMode(autoexec, &startmode);
		if(!(startmode & WCLOSE_START))
			allrewindow(SHOWTYPE);

		retcode = executer(startmode,TRUE,cmdline,NULL);

		return retcode;
	}
	else
		return 0;
}