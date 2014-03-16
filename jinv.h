#ifndef _JINV_H
#define _JINV_H

#include<stdlib.h>
#include<string.h>
#include<ncurses.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<errno.h>

#define SLAB_SIZE 100000		//the size of a single allocation
#define MAX_SIZE 16
#define MAX_COMMAND_LENGTH 5
#define MAX_FILE_NAME 128
#define LINE 1
#define ATOM 0
#define SCREEN_STATUS BUFFER_SCREEN.max_y-1
//#define MAX_FILE_NAME 20
#define JERK_NOP (char)-1
#define ATOM_SIZE sizeof(struct atomic_buffer)
#define LINE_SIZE sizeof(struct line_buffer)
#define JINV_TRUE 1
#define JINV_FALSE 0
#define SCROLL_UP 1
#define SCROLL_DOWN 0
#define SCROLL_PARTIAL 2
#define WRITE_BUFFER_SIZE 100

struct line_buffer
{
	int line_number;
	short char_count;
	int screen_line;//only to be changed by display management functions	//it marks whether the line is displayed on the screen or not if not displayed then -1 else any unsigned int 0 or greater with less than max_y of BUFFER_SCREEN
	struct line_buffer *previous_line;
	struct line_buffer *next_line;
	struct atomic_buffer *first_atom;
};
struct atomic_buffer
{
	struct atomic_buffer *previous_atom;
	struct atomic_buffer *next_atom;
	char data[MAX_SIZE];
};
struct current_position
{
	int max_x;
	int max_y;
	short current_column;
	//short screen_line;		//it marks the start of the screen line of the current line
	short topline_screen;		//it marks the topline of the screen
	short currentline_screen;//(in case of wraped line,the line is where the cursor is)	//it marks the current line of the screen and inside screen_display function it is same as bottomline_screen but is different when in edit mode i.e when we navigate
	short bottomline_screen;	//it marks the line where the text on the screen ends
	short current_char_count;	//it keeps the tap on the current char number in that line
	//char *current_char;		its not required
	short current_index;			//keeps the tap on the index number of the current char in the CURRENT_ATOM
	struct line_buffer *topline_buffer;
	struct line_buffer *bottomline_buffer;
};

//FILE *tmp_jerk=NULL;			//the temporary file where the buffer is to written to ensure loss of data NEED TO CHANGE THE FILENAME EVERY WHERE ELSE

int end_buffer(int);
int get_permissions(int);

struct line_buffer *give_line(void *,struct line_buffer *);	//allocates a chunk of line-structure and the first atomic structure along with the required settings of pointers from our own memory
struct atomic_buffer *give_atom(int,void *,void *);	//allocates a chunk of atomic structure along with the required settings of pointers from our own memory

struct line_buffer *current_first_line(void);
int set_flag_edit(void);
int end_screen(int);
int command_reader(char*,int);
int print_mesg(int,int,const char *);
int set_export(void *,void *,int,int);
int jinv_getmaxyx(void);
void jerk_flushinp(void);		//calls just flushinp() created for the same reason as jerk_getch()
int jerk_getch(void);
int initialise(int,char *); //does all the stuff before allowing the editor to start with command-mode like creation of sub-editor,screen-management,setting of few global parameters
int command_mode(FILE *);		//switches into the command mode.and takes input
int edit_mode(void);	//switches into the edit mode. and takes input
int move_cursor(int);	//moves the cursor in the screen depending on the current position
int redisplay_line(int);	//redisplays the current line edited.
int import_pointer(FILE *,void *);
int screen_refresh_set(int,void *);

#endif
