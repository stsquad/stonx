/*
** MONITOR externals and globals
**
** Include this if you want to add hooks into monitor code
*/

/*
** External signal types (probably needs to pass a structure of some sort)
*/ 

typedef enum monitor_signal_tag {
  BREAK=0x01,                      /* Shift-Pause from X  */
  GENERAL_EXCEPTION=0x02,          /* Processor exception */
  GEMDOS=0x04,                     /* Gemdos call */
  VBL=0x08                         /* VBL Interrupt */
}monitor_signal_type;

extern void init_monitor (int);
extern void signal_monitor (monitor_signal_type,void *);
extern int update_monitor (UL *,int,int);

extern int in_monitor;
