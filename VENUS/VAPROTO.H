/*
 * @(#) Gemini\vaproto.h
 * @(#) Stefan Eissing, 10. April 1991
 *
 * description: Definition der Nachrichten des Venus <-> Accessory
 * Protokolls
 */

#ifndef __vaproto__
#define __vaproto__

/* Message-Nummern f�r das xAcc-Protokoll von Konrad Hinsen
 * Venus gibt bei ACC_ID "VENUS.APP" und Gemini gibt "GEMINI.APP"
 * zur�ck. Gemini unterst�tzt xAcc der Stufe 0.
 */
#define ACC_ID		0x400
#define ACC_OPEN	0x401
#define ACC_CLOSE	0x402
#define ACC_ACC		0x403

/* Message-Nummern f�r die Kommunikation zwischen Venus und
 * verschiedenen Accesories.
 * Sollten in Nachrichten Pfade oder Dateinamen vorkommen, so
 * mu� immer der absolute Pfad angegeben werden (also mit Laufwerk)
 * und alle Buchstaben m�ssen Gro�buchstaben sein. Pfade enden
 * IMMER mit einem Backslash!
 *
 * Nachrichten von Venus beginnen mit VA (Venus -> Accessory).
 * Nachrichten an Venus beginnen mit AV (Accessory -> Venus).
 *
 * Mit AV_PROTOKOLL kann jedes Acc nachfragen, welche Nachrichten
 * verstanden werden (Dies ist ja bei VENUS und GEMINI 
 * unterschiedlich! Es w�re sch�n, wenn auch andere Programme auf
 * dieses Protokoll reagieren w�rden. Zumindest AV_SENDKEY ist
 * sicher leicht zu implementieren und ist die einzige Methode, wie
 * �ber Nachrichten Tastaturdr�cke simuliert werden k�nnen.
 */

/* AV_PROTOKOLL: Mit dieser Nachrichtennummer sollte man bei 
 * anderen Applikationen und auch VENUS/GEMINI nachfragen, ob
 * und welche Nachrichten sie versteht.
 */
#define AV_PROTOKOLL		0x4700
/*
 * Word 6+7: Pointer auf den Accessorynamen, wie er bei
 *           appl_find benutzt werden mu�; also 8 Zeichen lang
 *           nullterminiert (char name[9]).
 * Die Bits in den Worten 3, 4 und 5 haben folgende Bedeutung:
 * Word 3:
 * Bit 0:		(VA_SETSTATUS)
 * Bit 1:		(VA_START)
 * Alle anderen Bits sind f�r Erweiterungen reserviert und sollten
 * daher mit 0 vorbesetzt werden. Das gilt nat�rlich auch f�r die
 * Bits in den Worten 4 und 5.
 */
 
/* VA_PROTOSTATUS: Dem Sender von AV_PROTOKOLL wird mitgeteilt, da�
 * der Empf�nger etwas von diesem Protokoll wei�. Die Worte 3-7 des
 * Nachrichtenpuffers enthalten die Information, welche Nachrichten
 * verstanden werden. Gesetzte Bits stehen daf�r, da� eine Nachricht
 * (Nachrichtengruppe) verstanden werden. 
 */
#define VA_PROTOSTATUS		0x4701
/*
 * Word 6+7: Pointer auf den Programmnamen, wie er bei
 *           appl_find benutzt werden mu�; also 8 Zeichen lang
 *           nullterminiert (char name[9]).
 * Die Bits in den Worten 3, 4 und 5 haben folgende Bedeutung:
 * Word 3:
 * Bit 0		(AV_SENDKEY)
 * Bit 1		(AV_ASKFILEFONT)
 * Bit 2		(AV_ASKCONFONT, AV_OPENCONSOLE)
 * Bit 3		(AV_ASKOBJECT)
 * Bit 4		(AV_OPENWIND)
 * Bit 5		(AV_STARTPROG)
 * Bit 6		(AV_ACCWINDOPEN, AV_ACCWINDCLOSED)
 * Bit 7		(AV_STATUS, AV_GETSTATUS)
 * Bit 8		(AV_COPY_DRAGGED)
 * Alle anderen Bits sind f�r Erweiterungen reserviert und sollten
 * daher mit 0 vorbesetzt werden. Das gilt nat�rlich auch f�r die
 * Bits in den Worten 4 und 5.
 *
 * AV_SENDKEY kann sicher jeder leicht in seine Programme einbauen.
 * Bei AV_OPENWIND k�nnte ein Hauptprogramm auch seine "normale"
 * Routine zum �ffnen eines Dokumentes anwerfen und dabei den
 * �bergebenen Pfad benutzen. Dies ist zusammen mit der Benutzung
 * von TreeView sicher eine einfache Art, Dateien aus anderen Ordnern
 * oder Laufwerken zu laden.
 */

/* AV_GETSTATUS: Ein Accessory erfragt bei Venus den aktuellen
 * Status, den es Venus mit AV_STATUS mal gegeben hat.
 */
#define AV_GETSTATUS		0x4703

/* AV_STATUS: Ein Accessory kann Venus seinen Status mitteilen,
 * der dann von Venus im INF-File gespeichert wird und mit
 * AV_GETSTATUS wieder abgerufen werden kann.
 * Zuvor MUSS es sich aber mit AV_PROTOKOLL anmelden!
 * Word 3+4: Pointer auf einen String, der keine Steuerzeichen
 *           enthalten darf und nicht l�nger als 256 Zeichen
 *           sein darf. Dieser Pointer darf allerdings NULL sein.
 */
#define AV_STATUS			0x4704

/* VA_SETSTATUS: Venus teilt dem Accessory den abgespeicherten
 * Status bei Nachfrage durch AV_GETSTATUS mit. Dieser kann dann 
 * von einem Accessory gesetzt werden.
 * Word 3+4: Pointer auf einen String, der keine Steuerzeichen
 *           enth�lt.
 *           Dieser Pointer kann allerdings NULL sein, dann war
 *           kein Status gespeichert.
 */
#define VA_SETSTATUS		0x4705

/* AV_SENDKEY: Ein Acc sendet VENUS/GEMINI einen Tastaturevent, den
 * es selber vielleicht nicht versteht.
 * Word 3 = Tastaturstatus                 ev_mmokstate
 * Word 4 = Scancode der gedr�ckten Taste  ev_mkreturn
 */
#define	AV_SENDKEY			0x4710

/* VA_START: Accessory wird aktiviert. Word 3 + 4 enthalten einen
 * Pointer auf eine Kommandozeile, der auch NULL sein kann.
 * In der Kommandozeile stehen Pfade oder Dateinamen.
 */
#define VA_START			0x4711

/* AV_ASKFILEFONT: Frage nach dem eingestellten Zeichensatz
 * f�r Dateinamen.
 */
#define AV_ASKFILEFONT		0x4712

/* VA_FILEFONT: Gibt den derzeit eingestellten Zeichensatz.
 * Word 3 = Dateifontnummer (font id)
 * Word 4 = Dateifontgr��e (in points)
 */
#define VA_FILEFONT			0x4713

/* (Nur Gemini) AV_ASKCONFONT: Frage nach dem eingestellten 
 * Zeichensatz f�r das Console-Fenster.
 */
#define AV_ASKCONFONT		0x4714

/* VA_CONFONT: Gibt den derzeit eingestellten Zeichensatz.
 * Word 3 = Consolefontnummer (font id)
 * Word 4 = Consolefontgr��e (in points)
 */
#define VA_CONFONT			0x4715

/* AV_ASKOBJECT: Fragt nach dem derzeit selektiertem Objekt.
 * Es wird der Name des derzeit selektierten Objektes zur�ckgegeben.
 * Ist kein Objekt selektiert, so ist der String leer.
 * Sind mehrere Objekte selektiert, so sind ihre Namen durch Leer-
 * zeichen getrennt.
 */
#define AV_ASKOBJECT	0x4716

/* VA_OBJECT: Gibt Namen der derzeit selektierten Objekte.
 * Aufbau wie bei VA_START
 */
#define VA_OBJECT		0x4717

/* (Nur Gemini)AV_OPENCONSOLE: Venus soll das Console-Fenster �ffnen.
 * Ist es schon offen, so wird es nach vorne gebracht. Diese Aktion
 * ist vor allen Dingen sinnvoll, wenn ein Accessory ein TOS-Programm
 * mittels der system()-Funktion starten will (Warnung: Auf keinen
 * Fall darf ein GEM-Programm von einem Accessory via system() 
 * gestartet werden! (siehe auch AV_STARTPROG)
 * Auch sollte diese Nachricht nur auf ausdr�cklichen Wunsch des
 * Benutzers gebraucht werden, da es ihn sonst nur verwirren kann.
 *
 * ACHTUNG: Diese Nachricht steht nur in Gemini.app zur Verf�gung.
 */
#define AV_OPENCONSOLE	0x4718

/* VA_CONSOLEOPEN: Gibt zur�ck, ob das Console-Fenster nach vorne
 * gebracht worden ist. Word 3 == 0 (nein) != 0 (ja)
 */
#define VA_CONSOLEOPEN	0x4719

/* AV_OPENWIND: Venus soll ein Datei-Fenster �ffnen.
 * Dies sollte auch nur geschehen, wenn die Ursache f�r den
 * Benutzer ersichtlich ist.
 * Word 3+4 (Pointer) Pfad f�r das Fenster (s.o.).
 * Word 5+6 (Pointer) Wildcard f�r darzustellende Dateien.
 */
#define AV_OPENWIND		0x4720

/* VA_WINDOPEN: Gibt an, ob das Fenster ge�ffnet werden konnte.
 * siehe VA_CONSOLEOPEN
 */
#define VA_WINDOPEN		0x4721

/* AV_STARTPROG: Venus soll ein Programm starten. Hierbei
 * werden die angemeldeten Applikationen der Venus mit
 * ber�cksichtigt. Man kann also auch eine Datei angeben, f�r
 * die Venus dann ein Programm sucht.
 * Word 3+4 (Pointer) Programmname mit kompletten Pfad
 * Word 5+6 (Pointer) Kommandozeile (kann NULL sein)
 */
#define AV_STARTPROG	0x4722

/* VA_PROGSTART: Gibt an, ob Venus das Programm startet.
 * Word 3 == 0: nicht gestartet, != 0 gestartet
 * Im Allgemeinen wird das Acc. im Fehlerfall sofort eine
 * Nachricht bekommen. Wenn das Programm aber gestartet wird, erh�lt
 * das Acc. diese Nachricht erst nach dem Start des Programms, da
 * die Routine, die den Pexec macht nicht mehr wissen kann, das ein
 * Acc. noch schnell eine Nachricht bekommen mu�. Bei einem GEM-
 * Programm, kann man der Erfolg auch an dem AC_CLOSE erkennen.
 * Auch ist die Fehlererkennung nicht optimal. Der R�ckgabewert weist
 * auch nicht aus, das das Programm fehlerfrei gelaufen ist.
 */
#define VA_PROGSTART	0x4723

/* AV_ACCWINDOPEN: Mit dieser Nachricht kann ein Acc Venus mitteilen, da�
 * es ein Fenster ge�ffnet hat.
 * Word 3 AES-Handle des ge�ffneten Fensters
 */
#define AV_ACCWINDOPEN	0x4724

/* VA_DRAGACCWIND: Venus teilt dem Acc mit, da� Objekte auf eines seiner
 * mittels AV_ACCWINDOPEN angemeldeten Fenster gezogen worden sind.
 * Word 3   AES-Handle des Fensters
 * Word 4   X-Position der Maus
 * Word 5   Y-Position der Maus
 * Word 6+7 Pointer auf einen String, der die Namen der Objekte enth�lt.
 */
#define VA_DRAGACCWIND	0x4725

/* AV_ACCWINDCLOSED: Acc teilt Venus mit, da� sein Fenster geschlossen
 * wurde. Dies braucht das Acc nur in dem Fall zu tun, wenn es selbst das
 * Fenster schlie�t. Bekommt es eine AC_CLOSE Mitteilung vom AES, so wei�
 * Venus schon, da� alle Fenster weg sind.
 * Word 3   AES-Handle des Fensters
 */
#define AV_ACCWINDCLOSED	0x4726


/* Neu seit dem 11.04.1991!!!
 */
 
/* AV_COPY_DRAGGED: Accessorie teilt Venus mit, da� die Objekte,
 * die auf sein Fenster gezogen wurden, zu kopieren sind.
 * Dies kann z.B. nach dem Ziehen von Objekten auf das TreeView-
 * Fenster erw�nscht sein. Diese Nachricht ist nur als Antwort
 * auf VA_DRAGACCWIND gedacht.
 * Status: noch nicht implementiert.
 * Word 3	Tastaturstatus (Alternate, Control, Shift), der bei
 *          der VA_DRAGACCWIND-Nachricht �bergeben wurde
 * Word 4+5	Pointer auf einen String, der den Namen des Zielobjektes 
 *			enth�lt. Dies *mu�* ein Pfad sein!
 */
#define AV_COPY_DRAGGED		0x4728

/* VA_COPY_COMPLETE: Antwort auf AV_COPY. 
 * Word 3	Status des Kopierens. (!= 0 hei�t, da� wirklich etwas
 *			kopiert oder verschoben wurde. Dies kann das Acc evtl.
 *          zum Neuaufbau seines Fensters nutzen.)
 */
#define VA_COPY_COMPLETE	0x4729

#endif