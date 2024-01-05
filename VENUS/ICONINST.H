/*
 * iconinst.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for iconinst.c
 *
 * last change: 21.01.1991
 */

void copyNewDesk(OBJECT *pnewdesk);
void removeDeskIcon(word objnr);
void instShredderIcon(word obx, word oby, const char *name, char truecolor);
void instTrashIcon(word obx, word oby, const char *name,
					word shortcut, char truecolor);
void instScrapIcon(word obx, word oby, const char *name,
					word shortcut, char truecolor);
void instDriveIcon(word obx, word oby, word todraw, word iconNr,
					char drive, const char *name, word shortcut,
					char truecolor);
void instPrgIcon(word obx, word oby, word todraw, word normicon,
				word isfolder, char *path, const char *name,
				char *label, word shortcut);
void addDefIcons(void);
void doInstDialog(void);
word writeDeskIcons(word fhandle, char *buffer);
void instDraggedIcons(WindInfo *fwp,
				word fromx, word fromy, word tox, word toy);
void doShredderDialog(IconInfo *pii, word objnr);
void freeDeskTree(void);
void rehashDeskIcon(void);
word sortDeskTree(void);

void DeskIconNewLabel(char drive, const char *oldlabel, 
						const char *newlabel);
