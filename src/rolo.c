#include <sys/file.h>
#include <stdio.h>
#include <ctype.h>
#include <sgtty.h>
#include <signal.h>
#include <pwd.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sys5.h"

#ifdef TMC
#include <ctools.h>
#else
#include "ctools.h"
#endif

// #include "args.h"
#include "menu.h"
#include "mem.h"

#include "rolofiles.h"
#include "rolodefs.h"
#include "datadef.h"
#include "functions.h"

#define MAX_OPTIONS 52                 /* a-z A-Z */

static char rolodir[DIRPATHLEN];        /* directory where rolo data is */
static char filebuf[DIRPATHLEN];        /* stores result of homedir() */

int changed = 0;
int reorder_file = 0;
int rololocked = 0;
int in_search_mode = 0;


char *rolo_emalloc (size) int size;

/* error handling memory allocator */

{
  char *rval;
  if (0 == (rval = malloc(size))) {
     fprintf(stderr,"Fatal error:  out of memory\n");
     save_and_exit(-1);
  }
  return(rval);
}


char *copystr (s) char *s;

/* memory allocating string copy routine */

{
 char *copy;
 if (s == 0) return(0);
 copy = rolo_emalloc(strlen(s)+1);
 strcpy(copy,s);
 return(copy);
}


char *timestring ()

/* returns a string timestamp */

{
  char *s;
  long timeval;
  time(&timeval);
  s = ctime(&timeval);
  s[strlen(s) - 1] = '\0';
  return(copystr(s));
}


void user_interrupt ()

/* if the user hits C-C (we assume he does it deliberately) */

{
  unlink(homedir(ROLOLOCK));
  fprintf(stderr,"\nAborting rolodex, no changes since last save recorded\n");
  exit(-1);
}


void user_eof ()

/* if the user hits C-D */

{
  unlink(homedir(ROLOLOCK));
  fprintf(stderr,"\nUnexpected EOF on terminal. Saving rolodex and exiting\n");
  save_and_exit(-1);
}


void roloexit (rval) int rval;
{
  if (rololocked) unlink(homedir(ROLOLOCK));
  exit(rval);
}


void save_to_disk ()

/* move the old rolodex to a backup, and write out the new rolodex and */
/* a copy of the new rolodex (just for safety) */

{
  FILE *tempfp,*copyfp;
  char d1[DIRPATHLEN], d2[DIRPATHLEN];

  tempfp = fopen(homedir(ROLOTEMP),"w");
  copyfp = fopen(homedir(ROLOCOPY),"w");
  if (tempfp == NULL || copyfp == NULL) {
     fprintf(stderr,"Unable to write rolodex...\n");
     fprintf(stderr,"Any changes made have not been recorded\n");
     roloexit(-1);
  }
  write_rolo(tempfp,copyfp);

  /*check if not written */
  if(fclose(tempfp) != 0 || fclose(copyfp) != 0){
    roloexit(-1);
  }

  if (rename(strcpy(d1,homedir(ROLODATA)),strcpy(d2,homedir(ROLOBAK))) ||
      rename(strcpy(d1,homedir(ROLOTEMP)),strcpy(d2,homedir(ROLODATA)))) {
     fprintf(stderr,"Rename failed.  Revised rolodex is in %s\n",ROLOCOPY);
     roloexit(-1);
  }
  printf("Rolodex saved\n");
  sleep(1);
  changed = 0;
}


void save_and_exit (rval) int rval;
{
  if (changed) save_to_disk();
  roloexit(rval);
}


extern struct passwd *getpwnam();

char *home_directory (name) char *name;
{
  struct passwd *pwentry;
  if (0 == (pwentry = getpwnam(name))) return("");
  return(pwentry -> pw_dir);
}


char *homedir (filename) char *filename;

/* e.g., given "rolodex.dat", create "/u/massar/rolodex.dat" */
/* rolodir generally the user's home directory but could be someone else's */
/* home directory if the -u option is used. */

{
  nbuffconcat(filebuf,3,rolodir,"/",filename);
  return(filebuf);
}


char *libdir (filename) char *filename;

/* return a full pathname into the rolodex library directory */
/* the string must be copied if it is to be saved! */

{
  nbuffconcat(filebuf,3,ROLOLIB,"/",filename);
  return(filebuf);
}


void locked_action (int x)
{
  if (x == 3) {
     fprintf(stderr,"Someone else is modifying that rolodex, sorry\n");
     exit(-1);
  }
  else {
     cathelpfile(libdir("lockinfo"),"locked rolodex",0);
     exit(-1);
  }
}


void rolo_main(int argc, char *argv[]){

    int fd,in_use,read_only,rolofd;
    int flag = 0;
    char *user;
    FILE *tempfp;

    int argcount = 0;

    clearinit();
    clear_the_screen();

    int option;
    while((option = getopt(argc, argv, "lsu:")) != -1){
      switch(option){
        case 'l':
          flag = 1;
          break;
        case 's':
          flag = 2;
          break;
        case 'u':
          user = optarg;
          flag = 3;

          break;
        default:
          roloexit(-1);
      }
    }

    /* memory to store options other than flags */
    char **names = malloc(sizeof(char*)*MAX_OPTIONS);
    int j = 0;
    for(int i = optind; i < argc; i++){
      *(names+j) = malloc(sizeof(char)*strlen(argv[i]));
      strcpy(*(names+j), argv[i]);
      j++;
      argcount++;
    }



    /* find the directory in which the rolodex file we want to use is */



    if(flag != 3){
       if (0 == (user = getenv("HOME"))) {
          fprintf(stderr,"Cant find your home directory, no HOME\n");
          roloexit(-1);
       }
       strcpy(rolodir,user);
    }

    if (flag == 3) {
       strcpy(rolodir,home_directory(user));
       if (*rolodir == '\0') {
          fprintf(stderr,"No user %s is known to the system\n",user);
          roloexit(-1);
       }
    }

    /* is the rolodex readable? */

    if (0 != access(homedir(ROLODATA),R_OK)) {

       /* No.  if it exists and we cant read it, that's an error */

       if (0 == access(homedir(ROLODATA),F_OK)) {
          fprintf(stderr,"Cant access rolodex data file to read\n");
          roloexit(-1);
       }

       /* if it doesn't exist, should we create one? */

       if (flag == 3) {
          fprintf(stderr,"No rolodex file belonging to %s found\n",user);
          roloexit(-1);
       }

       /* try to create it */

       if (-1 == (fd = creat(homedir(ROLODATA),0644))) {
          fprintf(stderr,"couldnt create rolodex in your home directory\n");
          roloexit(-1);
       }

       else {
          close(fd);
          fprintf(stderr,"Creating empty rolodex...\n");
       }

    }

    /* see if someone else is using it */

    in_use = (0 == access(homedir(ROLOLOCK),F_OK));

    /* are we going to access the rolodex only for reading? */

    if (!(read_only = (flag == 2) || argcount > 0)) {

       /* No.  Make sure no one else has it locked. */

       if (in_use) {
          locked_action(flag);
       }

       /* create a lock file.  Catch interrupts so that we can remove */
       /* the lock file if the user decides to abort */

       if (flag != 1) {
          if ((fd = open(homedir(ROLOLOCK),O_EXCL|O_CREAT,00200|00400)) < 0) {
             fprintf(stderr,"unable to create lock file...\n");
              exit(1);
    }
          rololocked = 1;
          close(fd);
          signal(SIGINT,user_interrupt);
       }

       /* open a temporary file for writing changes to make sure we can */
       /* write into the directory */

       /* when the rolodex is saved, the old rolodex is moved to */
       /* a '~' file, the temporary is made to be the new rolodex, */
       /* and a copy of the new rolodex is made */

       if (NULL == (tempfp = fopen(homedir(ROLOTEMP),"w"))) {
           fprintf(stderr,"Can't open temporary file to write to\n");
           roloexit(-1);
       }
       fclose(tempfp);

    }

    allocate_memory_chunk(CHUNKSIZE);

    if (-1 == (rolofd = open(homedir(ROLODATA),O_RDONLY))) {
        fprintf(stderr,"Can't open rolodex data file to read\n");
        roloexit(-1);
    }

    /* read in the rolodex from disk */
    /* It should never be out of order since it is written to disk ordered */
    /* but just in case... */

    if (!read_only) printf("Reading in rolodex from %s\n",homedir(ROLODATA));
    read_rolodex(rolofd);
    close(rolofd);
    if (!read_only) printf("%d entries listed\n",rlength(Begin_Rlist));
    if (reorder_file && !read_only) {
       fprintf(stderr,"Reordering rolodex...\n");
       rolo_reorder();
       fprintf(stderr,"Saving reordered rolodex to disk...\n");
       save_to_disk();
    }

    /* the following routines live in 'options.c' */

    /* -s option.  Prints a short listing of people and phone numbers to */
    /* standard output */

    if (flag == 2) {
        print_short();
        exit(0);
    }

    /* rolo <name1> <name2> ... */
    /* print out info about people whose names contain any of the arguments */

    if (argcount > 0) {
       print_people(names);
       exit(0);
    }

    /* regular rolodex program */

    interactive_rolo();
    exit(0);

}

