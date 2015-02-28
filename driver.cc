/********************************************************************
* Description: driver.cc
*   Drives the interpreter from a menu based user interface.
*
* 13-Oct-2000 WPS changed gets to fgets and moved external canon variable
* definitions to canon.hh. (This may be temporary.)
* Early March 2007 MGS adapted this to emc2
*
*   Derived from a work by Tom Kramer
*
* Author:
* License: GPL Version 2
* System: Linux
*
* Copyright (c) 2007 All rights reserved.
*
* Last change:
*
********************************************************************/

#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include "rs274ngc_return.hh"
#include "inifile.hh"		// INIFILE
#include "canon.hh"		// _parameter_file_name
#include "config.h"		// LINELEN
#include "tool_parse.h"
#include <stdio.h>    /* gets, etc. */
#include <stdlib.h>   /* exit       */
#include <string.h>   /* strcpy     */
#include <getopt.h>

Interp interp_new;
//Inifile inifile;

#define active_settings  interp_new.active_settings
#define active_g_codes   interp_new.active_g_codes
#define active_m_codes   interp_new.active_m_codes
#define error_text	 interp_new.error_text
#define interp_execute	 interp_new.execute
#define file_name	 interp_new.file_name
#define interp_init	 interp_new.init
#define stack_name	 interp_new.stack_name
#define line_text	 interp_new.line_text
#define line_length	 interp_new.line_length
#define sequence_number  interp_new.sequence_number
#define interp_close     interp_new.close
#define interp_exit      interp_new.exit
#define interp_open      interp_new.open
#define interp_read	 interp_new.read
#define interp_load_tool_table interp_new.load_tool_table

/*

This file contains the source code for an emulation of using the six-axis
rs274 interpreter from the EMC system.

*/

/*********************************************************************/

/* report_error

Returned Value: none

Side effects: an error message is printed on stderr

Called by:
  interpret_from_file
  interpret_from_keyboard
  main

This

1. calls error_text to get the text of the error message whose
code is error_code and prints the message,

2. calls line_text to get the text of the line on which the
error occurred and prints the text, and

3. if print_stack is on, repeatedly calls stack_name to get
the names of the functions on the function call stack and prints the
names. The first function named is the one that sent the error
message.


*/

void report_error( /* ARGUMENTS                            */
 int error_code,   /* the code number of the error message */
 int print_stack)  /* print stack if ON, otherwise not     */
{
  char interp_error_text_buf[LINELEN];
  int k;

  error_text(error_code, interp_error_text_buf, 5); /* for coverage of code */
  error_text(error_code, interp_error_text_buf, LINELEN);
  fprintf(stderr, "%s\n",
          ((interp_error_text_buf[0] == 0) ? "Unknown error, bad error code" : interp_error_text_buf));
  line_text(interp_error_text_buf, LINELEN);
  fprintf(stderr, "%s\n", interp_error_text_buf);
  if (print_stack == ON)
    {
      for (k = 0; ; k++)
        {
          stack_name(k, interp_error_text_buf, LINELEN);
          if (interp_error_text_buf[0] != 0)
            fprintf(stderr, "%s\n", interp_error_text_buf);
          else
            break;
        }
    }
}

/***********************************************************************/

/* interpret_from_keyboard

Returned Value: int (0)

Side effects:
  Lines of NC code entered by the user are interpreted.

Called by:
  interpret_from_file
  main

This prompts the user to enter a line of rs274 code. When the user
hits <enter> at the end of the line, the line is executed.
Then the user is prompted to enter another line.

Any canonical commands resulting from executing the line are printed
on the monitor (stdout).  If there is an error in reading or executing
the line, an error message is printed on the monitor (stderr).

To exit, the user must enter "quit" (followed by a carriage return).

*/

int interpret_one_line(  /* ARGUMENTS                 */
 int block_delete,            /* switch which is ON or OFF */
 int print_stack,
 char *line)             /* option which is ON or OFF */
{
	int status;
	status = interp_read(line);
	if ((status == INTERP_EXECUTE_FINISH) && (block_delete == ON));
	else if (status == INTERP_ENDFILE);
	else if ((status != INTERP_EXECUTE_FINISH) &&
			 (status != INTERP_OK))
	  report_error(status, print_stack);
	else
	  {
		status = interp_execute();
		if ((status == INTERP_EXIT) ||
			(status == INTERP_EXECUTE_FINISH));
		else if (status != INTERP_OK)
		  report_error(status, print_stack);
	  }
}

/************************************************************************/

/* read_tool_file

Returned Value: int
  Returns 0 for success, nonzero for failure.  Failures can be caused by:
  1. The file named by the user cannot be opened.
  2. Any error detected by loadToolTable()

Side Effects:
  Values in the tool table of the machine setup are changed,
  as specified in the file.

Called By: main
*/

int read_tool_file(  /* ARGUMENTS         */
 const char * tool_file_name)   /* name of tool file */
{
  char buffer[1000];

  if (tool_file_name[0] == 0) /* ask for name if given name is empty string */
    {
      fprintf(stderr, "name of tool file => ");
      fgets(buffer, 1000, stdin);
      buffer[strlen(buffer) - 1] = 0;
      tool_file_name = buffer;
    }

  return loadToolTable(tool_file_name, _tools, 0, 0, 0);
}


int main (int argc, char ** argv)
{
	int status;
	int block_delete = OFF;
	int print_stack = OFF;
	char line[LINELEN];

	if (read_tool_file(TOOL_CONFIGURE_PATH) != 0)
		exit(1);

	if ((status = interp_init()) != INTERP_OK)
	{
		report_error(status, print_stack);
		exit(1);
	}

	for(; ;)
	{
		char *result;
		printf("READ => ");
		result = fgets(line, LINELEN, stdin);
		if (!result || strcmp (line, "quit\n") == 0)
			return 0;
		interpret_one_line(block_delete, print_stack, line);
	}
	return 0;
}

