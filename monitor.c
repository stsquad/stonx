/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 *
 * The old monitor code has been heavily butchered due to a mixture
 * of wierd ncurses stuff and not understanding the OPCODE generator output.
 * The original (broken) code can be found in release prior to 6.7.3
 */

/* Standard Includes */
#include <stdio.h>
#include <ctype.h>

#include "defs.h"
#include "tosdefs.h"
#include "mem.h"

#include "monitor.h"

#define WATCH 1

/* local global vars - used for various breakpoints */
static int in_monitor=0;

static UL count = 0;
static UL bkpt = 0;

static UL watch = 0;
static UW watch_val = 0;

static monitor_signal_type signal_break=BREAK;


/* For parsing cmd lines */
#define EQ(_x,_y) (strncasecmp(_x,_y,strlen(_y))==0)

#define NUMCMDS 12

typedef struct command_tag
{
   int             args;
   char           *cmdlist[NUMCMDS];
} command;

/* external vars */
extern int instruction_count;

/* local functions */
/* interactive */
static void print_help (void);
static void set_breakpoint (command *cmd);
static void dump_memory (command *cmd);
/* other */
static int dissassemble (int pos, int count);
static void dump_registers (UL *regs, int sr);

/* disassembly routine */
extern void* dis (char *ptr,char *string);


void init_monitor (int startflag)
{
  if (startflag)
    {
      fprintf (stderr,"Monitor being initialised (break@start).....\n");
      in_monitor=1;
    } else {
      in_monitor=0;
    }
}

/*
** New function to signal events to monitor
** e.g. Exceptions, Shift-pause etc..
**
** At the moment it just sets the in_monitor flag for
** when the execute loop next gets round to us!
** 
** TODO: Actually define some conditions so we can filter on them
**
*/

void signal_monitor (monitor_signal_type reason,void *data)
{
  fprintf (stderr,"Signal to monitor %x\n",reason);

  if (reason&signal_break)
    {
      /* May put more advanced processing here?*/
      in_monitor=1;
    }
}

void kill_monitor (void)
{
  //endwin();
}

/*
** The update monitor function is called every instruction cycle
** It should exit straight away unless
**
** In the monitor state (in_monitor!=0)
**   1. the execution count has reached 0
**   2. the pc has hit a breakpoint address
**
** In the running state (in_monitor==0)
**   1. BAD: REMOVED see signal_monitor // an exception has been reached (signalled by the exception flag)
**
** Interesting vars:
**     cmd - string for processing debugger commands
**     count - count of monitor entries (used for executing n instructions)
*/

int update_monitor (UL *regs, int sr, int pcoff)
{
  int monitor=0;
  int exit_val=0;
  char cmd[100]; 
  command *cmdline;

	int i, j, x, c, cha=0, toppc, pos, lo1, eoi, yp, ddd=0;
	static UW v=0x4e71;
	static UL lv=0;
	UL u,s;

	/* Question? Would an OR test be optimised faster?
       ** need stateful logic here? what does in_monitor mean?*/
	if (in_monitor)
	  {
	    if (count-- > 0) return (exit_val);
	 }
	else if (watch || bkpt)
	  {
	    if (TRIM(pcoff) != TRIM(bkpt) &&
		LM_UW(MEM(watch)) == watch_val ) return (exit_val);
	  }    
	else
	  {
	    return (exit_val);
	  }/* endif !in_monitor */ 


	fprintf (stderr,"Entering Monitor after %d instructions\n",instruction_count);

	in_monitor=1;
	monitor=1;
	count = 0;
	dump_registers(regs,sr);
	dissassemble (pcoff,10);

	while (monitor)
	  {
	    fprintf(stdout,"\nSTMON>");

	    fgets(cmd,sizeof(cmd),stdin);
	    cmdline = parse_command (cmd);

	    switch (*(cmdline->cmdlist[0]))
	      {
	      case 'B':
	      case 'b':
		{
		  set_breakpoint(cmdline);
		  break;
		}
	      case 'D':
	      case 'd':
		{
		  dump_memory(cmdline);
		  break;
		}
	      case 'h':
	      case 'H':
	      case '?':
		{
		  print_help();
		  break;
		}
	      case 'Q':
	      case 'q':
		{
		  monitor=0;
		  exit_val=1;
		  break;
		}
	      case 'R':
	      case 'r':
		{
		  if (cmdline->args>1) 
		    {
		      count=strtol(cmdline->cmdlist[1],NULL,10);
		      monitor=0;
		    }
		  else
		    {
		      fprintf (stderr,"Need number of instructions to run!\n");
		    }
		  break;
		}
	      case 'S':
	      case 's':
		{
		  count=0;
		  monitor=0;
		  break;
		}
	      case 'g':
	      case 'G':
		{
		  monitor=0;
		  in_monitor=0;
		  break;
		}
	      case 'w':
	      case 'W':
		{
		  set_watch(cmdline);
		  break;
		}

	      default:
		break;
	      } /* End Switch */

	  } /* End While */
	return (exit_val);
}

/*
** Taken from sf snippet library (written by jholder)
** now returns pointer to alloc'ed memory instead of locally scoped variable.
** Also uses the strtok function instead of poorly reinventing it.
*/


command  *parse_command(char *cmd)
{
  char *word = NULL;
  char *sep = " ";
  int  nargs = 0;
  command *newcmd;

  newcmd = malloc( sizeof( command ) );
  if( !newcmd ) return NULL;
  memset( newcmd, 0, sizeof( command ) );

  for (word = strtok(cmd, sep);
       word;
       word = strtok(NULL, sep) )
  {
     newcmd->cmdlist[ nargs ] = word;
     nargs++;
     if( nargs > NUMCMDS-1 ) break;
  }
  newcmd->args = nargs;
  return newcmd;
}
		



/* 
** Dump Help for Debugger
**
*/

static void print_help (void)
{
  fprintf (stdout,"b <hex address> - break when pc=address\n");
  fprintf (stdout,"b <vbl|gemdos|exception> - break on condition\n");
  fprintf (stdout,"bc  - clear conditional breaks\n");
  fprintf (stdout,"r n - run n instructions\n");
  fprintf (stdout,"s   - step (one instruction)\n");
  fprintf (stdout,"g   - go (continue running)\n");
  fprintf (stdout,"w <address> - break if address contents change\n");
  fprintf (stdout,"q   - quit (leave STonX)\n");
}

/*
** Dump n bytes in a nice word format (with a newline)
*/

static dump_bytes (int ptr, int count)
{
  int i;

  for (i=0;i<(count/2);i++)
    { 
      fprintf (stdout,"%04x ", LM_UW(MEM(ptr+i)));
    }
  fprintf (stdout,"\n");
}

/*
** Dump Registers D0-D7/A0-A7
**
** TODO:
**  1. Proper decode of the SSR (In Hex?!)
*/

static void dump_registers (UL *regs, int sr)
{
  int i,j;
  UL u,s;

	for (i=0; i<8; i=i+4)
	{
	      fprintf (stdout,"D%d: %04x %04x ", i, regs[i]>>16, regs[i]&0xffff);
	      fprintf (stdout,"D%d: %04x %04x ", i+1, regs[i+1]>>16, regs[i+1]&0xffff);
	      fprintf (stdout,"D%d: %04x %04x ", i+2, regs[i+2]>>16, regs[i+2]&0xffff);
	      fprintf (stdout,"D%d: %04x %04x ", i+3, regs[i+3]>>16, regs[i+3]&0xffff);
	      fprintf (stdout,"\n");
	}
	for (i=0; i<8; i=i+4)
	{
	  fprintf (stdout,"A%d: %04x %04x ", i, regs[i+8]>>16, regs[i+8]&0xffff);
	  fprintf (stdout,"A%d: %04x %04x ", i+1, regs[i+9]>>16, regs[i+9]&0xffff);
	  fprintf (stdout,"A%d: %04x %04x ", i+2, regs[i+10]>>16, regs[i+10]&0xffff);
	  fprintf (stdout,"A%d: %04x %04x ", i+3, regs[i+11]>>16, regs[i+11]&0xffff);
	  fprintf (stdout,"\n");
	}
#if 0 /* broken?*/
        if (sr & 0x2000)
	{
	  fprintf (stdout, "Supervisor Mode:");
		u = regs[16]; s = regs[15];
	}
	else
	{
	  fprintf (stdout,"User Mode:");
		u = regs[15]; s = regs[16];
	}
	#endif
	fprintf (stdout, "SR:%x", sr);
#if 0 /* broken? */
	if (u&0x01) fprintf (stdout,"C");
	if (u&0x02) fprintf (stdout,"O");
	if (u&0x04) fprintf (stdout,"Z");
	if (u&0x08) fprintf (stdout,"N");
	if (u&0x10) fprintf (stdout,"X");
#endif
	fprintf (stdout,"\n");
}

/*
** Format STMON> d address [length]
**
** Check if d is a register alias (@d0-@a7,@sp)
** Or check if its a normal hex address
**
** Default to twenty words.
*/

static void dump_memory (command *cmd)
{

}


/*
** Set Break point
**
** Format STMON> b <address | type>
**
** Scan for an address or type (exception/gemdos/interupt?)
**
** Format STMON> bc
**
** Clear all signal break points
*/

static void set_breakpoint (command *cmd)
{

  if (cmd->args<2)
    {
      if (EQ(cmd->cmdlist[0],"BC"))
	{
	  /*bkpt=0;*/
	  signal_break = BREAK;
	} else {
	  fprintf (stderr,"Need at least an address or type\n");
	  fprintf (stderr,"Current pc bkpt is %x, signal mask %x\n",bkpt,signal_break);
	}
    }
  else
    {
      if (EQ(cmd->cmdlist[1],"VBL"))
	{
	  fprintf (stderr, "Setting break on next VBL interrupt\n");
	  signal_break = signal_break | VBL;
	} 
      else if (EQ(cmd->cmdlist[1],"EXCEPTION"))
	{
	  fprintf (stderr, "Setting break on next EXCEPTION\n");
	  signal_break = signal_break | GENERAL_EXCEPTION;
	}
      else /* assume its an address and try to parse it */
	{
	  bkpt = strtol (cmd->cmdlist[1],NULL,16);
	  fprintf (stderr, "Breakpoint set at %x\n", bkpt);
	}
      in_monitor=1;
    }
}

/*
** set watch
*/

static void set_watch (command *cmd)
{
  if (cmd->args<2)
    {
      if (EQ(cmd->cmdlist[0],"WC"))
	{
	  watch=0;
	} else {
	  fprintf (stderr,"Need at least an address or register\n");
	  fprintf (stderr,"Current watch address is %x, value %x\n",watch,watch_val);
	}
    }
  else
    {
      if (*cmd->cmdlist[1]=='@')
	{
	  /* eval register */
	}
      else
	{
	  watch = strtol (cmd->cmdlist[1],NULL,16);
	  watch_val = LM_UW(MEM(watch));
	  fprintf (stderr, "Watch set at %x, value currently %x\n", watch,watch_val);
	}
      in_monitor=1;
    }

}

/*
** The Original dissassembly used some table generated by the OPCODE routines
** This now uses a patched version of Jim Patchells dissassembler (patched to
** read memory using STonXs L_ macros) 
**
** TODO:
*/

static int dissassemble (int pos, int count)
{
  int i,j,k;
  int old_pos;
  char mem_string[10];
  char *local_mem;
  char *string,*string2;

  string=malloc(80);
  if (!string)
    { 
      free(string); 
      return; 
    }


  for (i=0; i<count; i++)
    {
      old_pos=pos;
      pos = dis ((char *) pos,string); /* UGLY Kludge */
      k = pos-old_pos;
      fprintf(stdout,"%-50s",string);

      fprintf(stdout,"[");
     for (j=0; j<k; j=j+2)
      {
	fprintf(stdout,"%04x ", LM_UW(MEM(old_pos+j)));
      }

      fprintf(stdout,"]\n");
   }

  // Free memory!
  free(string);

  return;
	
}


//#endif /* MONITOR */

