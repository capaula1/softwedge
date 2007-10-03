#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <X11/keysymdef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "softwedge.h"

int serialPort;

void xtest_key_press(Display *dpy, unsigned char letter) {
  unsigned int shiftcode = XKeysymToKeycode(dpy, XStringToKeysym("Shift_L"));
  int upper = 0;
  int skip_lookup = 0;
  char s[2];
  s[0] = letter;
  s[1] = 0;
  KeySym sym = XStringToKeysym(s);
  KeyCode keycode;



  if (sym == 0) {
    sym = letter;
  }


  if (sym == '\n') {
    sym = XK_Return;
    skip_lookup = 1;

  } else if (sym == '\t') {
    sym = XK_Tab;
    skip_lookup = 1;
  }

  keycode = XKeysymToKeycode(dpy, sym);
  if (keycode == 0) {
    sym = 0xff00 | letter;
    keycode = XKeysymToKeycode(dpy, sym);
    
  }

  if (!skip_lookup) {
    // Here we try to determine if a keysym
    // needs a modifier key (shift), such as a
    // shifted letter or symbol.
    // The second keysym should be the shifted char
    KeySym *syms;
    int keysyms_per_keycode;
    syms = XGetKeyboardMapping(dpy, keycode, 1, &keysyms_per_keycode);
    int i = 0;
    for (i = 0; i <= keysyms_per_keycode; i++) {
      if (syms[i] == 0)
	break;
      
      if (i == 0 && syms[i] != letter)
	upper = 1;
      
      
    }
  }

  if (upper)
    XTestFakeKeyEvent(dpy, shiftcode, True, 0);	

  
  XTestFakeKeyEvent(dpy, keycode, True, 0);	
  XTestFakeKeyEvent(dpy, keycode, False, 0);

  if (upper)
    XTestFakeKeyEvent(dpy, shiftcode, False, 0);	

  

}

void press_keys(Display *dpy, char* string) {
  int len = strlen(string);
  int i = 0;
  for (i = 0; i < len; i++) {
    xtest_key_press(dpy, string[i]);
  }
  XFlush(dpy);
}


int sw_open_serial(const char *port) {
  serialPort = open(port, O_RDONLY);
  if (serialPort < 0) {
    fprintf(stderr, "Can't open serial port: %s\n", port);
    exit(-1);
  }

  return 0;
}


int main(int argc, char**argv)
{
  Display    *dpy;            /* X server connection */
  int xtest_major_version = 0;
  int xtest_minor_version = 0;
  int dummy;
  int c;
  char *sport = NULL;

  while ((c = getopt (argc, argv, "vc:")) != -1)
    switch (c)
      {
      case 'v':
	fprintf(stderr, "softwedge v %s: The serial softwedge X11 helper. ", SOFTWEDGE_VERSION);
	fprintf(stderr, "(c) 2007 Yann Ramin <atrus@stackworks.net>\n(Exiting...)\n");
	exit(0);
      case 'c':
	sport = optarg;
	break;
      case '?':
	if (optopt == 'c')
	  fprintf (stderr, "Option -%c requires an argument.\n", optopt);
	else if (isprint (optopt))
	  fprintf (stderr, "Unknown option `-%c'.\n", optopt);
	else
	  fprintf (stderr,
		   "Unknown option character `\\x%x'.\n",
		   optopt);
	return 1;
      default:
	abort ();
      }

  if (sport == NULL)
    sport = DEFAULT_SERIAL;


  
  /*
   * Open the display using the $DISPLAY environment variable to locate
   * the X server.  See Section 2.1.
   */
  if ((dpy = XOpenDisplay(NULL)) == NULL) {
    fprintf(stderr, "%s: can't open %s\en", argv[0], XDisplayName(NULL));
    exit(1);
  }
  
  Bool success = XTestQueryExtension(dpy, &dummy, &dummy,
				     &xtest_major_version, &xtest_minor_version);
  if(success == False || xtest_major_version < 2 ||
     (xtest_major_version <= 2 && xtest_minor_version < 2))
    {
      fprintf(stderr,"XTEST extension not supported. Can't continue\n");
      exit(1);
    }
  

  sw_open_serial(sport);




  if(fork()) {
    return 0;
  }

  close(0);
  close(1);
  close(2);


  char readbuf[2];
  readbuf[1] = 0;

  while(read(serialPort, readbuf, 1) > 0) {

    press_keys(dpy, readbuf);

  }





  return 0;
}
