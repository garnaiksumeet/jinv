#include"jinv.h"

static int FILE_EXISTS=0; // 0 for YES 1 for NO.
static char FILE_NAME[MAX_FILE_NAME];	//name of the file being reads
static int RWPERM; //0 for read-only, 1 for Read-Write, -1 access denied
static unsigned int FILE_SIZE=0;	//contains the size of the file being viewed or edited
static struct line_buffer *FIRST_LINE=NULL;	//the pointer to the first line of the buffer
static struct line_buffer *CURRENT_LINE=NULL;		//the pointer to the current line in buffer
static struct atomic_buffer *CURRENT_ATOM=NULL;	//the pointer to the current atom in buffer
static struct atomic_buffer *LAST_CHUNK=NULL;		//the pointer to the last allocated chunk of memory from our local memory
static FILE *in; 			// input file pointer
static void *ptr=NULL;			//the pointer to the first location of entire chunk of memory
//need to write for the places where we need to allocate more space

static int check_file(void); //checks the file existence(FILE_EXISTS(it is set in the main function)) and RW permission(sets RWPERM variable) and opens the file in read mode if read permission given
static int set_buffer(void);	//allocates memory for the buffer and copies the file into buffer depending on RWPERM.
static int create_buffer(void);	//creates buffer based on RWPERM and file size/existence.
static int init_buffer_read(void);		//stores the input file into the buffer depending on FILE_EXISTS.
static int read_line_file(void);	//reads a single line into the buffer from the file
static int set_screen(void);	// initialises the curses lib and prints the buffer.


int initialise(int file_exists,char *file_name)		//all this function does is basically sets all the required memory structures and display requirements before handling over the control of program to the handler function
{
	FILE_EXISTS=file_exists;
	memset(FILE_NAME,'\0',MAX_FILE_NAME);
	strncpy(FILE_NAME,file_name,MAX_FILE_NAME);

	check_file();	      //depending on the invocation with or without filename it sets FILE_EXISTS and RWPERM.
	set_buffer();         //allocates memory for the buffer and copies the file into buffer depending on RWPERM.
	set_screen();         // initialises the curses lib and displays the buffer.
	import_pointer(in,ptr);
	return 0;
}
static int check_file(void)	//checks the file existence(FILE_EXISTS) and RW permission(sets RWPERM variable) and opens the file in read mode if read permission given
{
	if (0 == FILE_EXISTS)//THIS CHECKS FOR THE INVOCATION WITH FILE NAME OR NOT
	{
		FILE_EXISTS=access(FILE_NAME,F_OK);	//access is a system function of LINUX which returns the file permissions as requested(for more details regarding access() read its man-page.)
		if (0 == FILE_EXISTS)	//if the file exists then check for read-write permissions and open in read mode
		{
			if (0 == access(FILE_NAME,W_OK))	//check for write permission
			{
				struct stat file;
				stat(FILE_NAME,&file);
				FILE_SIZE=((int)file.st_size +SLAB_SIZE);
				RWPERM=1;
			}
			else//if write permission is denied
			{
				if (0 == access(FILE_NAME,R_OK))	//check for read permission
				{
					struct stat file;
					stat(FILE_NAME,&file);	//stat is a system defined structure that stores details regarding a file including the size(in bytes).For more details see man-pages
					FILE_SIZE=((int)file.st_size + (SLAB_SIZE/20));	//check for more appropriate numbers
					RWPERM=0;
				}
				else
				{
					RWPERM=-1;
					return 1;
				}
			}
			in=fopen(FILE_NAME,"r");
			/*if (0 == RWPERM)
				return 0;
			memset(jerk_tmp_file,'\0',(MAX_FILE_NAME + 10));
			strncpy(jerk_tmp_file,FILE_NAME,(MAX_FILE_NAME - 1));
			strncat(jerk_tmp_file,"XXXXXX",7);
			fildes=mkstemp(jerk_tmp_file);
			tmp=fdopen(fildes,"w");
			mode_t mode,tmp_mode;
			tmp_mode=((mode & S_IRWXU)|(mode & S_IRWXG)|(mode & S_IRWXO));
			chmod(jerk_tmp_file,tmp_mode);*/
		}//if the file does not exists then open a file with that name in write mode
		else//new file
		{
			in=fopen(FILE_NAME,"w");
			chmod(FILE_NAME,((S_IRUSR | S_IWUSR)|(S_IRGRP | S_IWGRP)|(S_IROTH)));
			FILE_EXISTS=1;
			FILE_SIZE=SLAB_SIZE;
		}
	}
	else
	{
		printf("invoked without file name\n");
		exit(1);
	}
	return 0;
}
static int set_buffer(void)	//allocates memory for the buffer and copies the file into buffer depending on RWPERM.
{
	create_buffer();	//creates buffer based on RWPERM and file size/existence.
	init_buffer_read();	//stores the input file into the buffer depending on FILE_EXISTS.
	CURRENT_LINE=FIRST_LINE;
	CURRENT_ATOM=CURRENT_LINE->first_atom;
	return 0;
}
static int create_buffer(void)
{
	if (-1 == RWPERM)		//if there is no read permission on the file name then it exits
		return 0;
	else
	{
/*allocates total memory of FILE_SIZE as computed by the check_file function,there is no requirement of casting as C89 returns a void pointer and the problem with casting is not including stdlib.h creeps in some error while dereferencing it.(google to know more)
*/
		ptr=malloc(FILE_SIZE);
/*FIRST_LINE is the pointer to the node pointing to the first line of the file and then the usual religion of assigning all the pointers in that structure to NULL and the line number to 1
*/
		FIRST_LINE=(struct line_buffer *)ptr;
		CURRENT_LINE=FIRST_LINE;
		FIRST_LINE->previous_line=NULL;
		FIRST_LINE->next_line=NULL;
		FIRST_LINE->line_number=1;
		FIRST_LINE->screen_line=-1;
/*its quite obvious that creating only a structure that points to line_buffer is useless until we allocate some memory for the first atom it points to,give_atom() does that job
*/
		FIRST_LINE->first_atom=give_atom(LINE,FIRST_LINE,CURRENT_ATOM);
		CURRENT_ATOM=CURRENT_LINE->first_atom;
		LAST_CHUNK=CURRENT_ATOM;
	}
	return 1;
}
static int init_buffer_read(void)		//stores the input file into the buffer depending on FILE_EXISTS.
{
/*checks for file existence and read-write permissions so as to read the file accordingly i.e if file is new/no read permission then it simply returns 1.
*/
	if (1 == FILE_EXISTS)
	{//when we invoke with a new file name
		CURRENT_ATOM->data[0]='\n';
		(CURRENT_LINE->char_count)++;
		CURRENT_LINE->next_line=give_line(LAST_CHUNK,CURRENT_LINE);
		return 1;
	}
	if (-1 == RWPERM)
		return 1;
/*read_line_file returns 0 after reading one line from the file into the buffer and 1 when it encounters EOF.the below two lines of code reads an entire file into the buffer.
*/
	while (1 != read_line_file())
	{
		CURRENT_LINE->next_line=give_line(CURRENT_ATOM,CURRENT_LINE);
		CURRENT_LINE=CURRENT_LINE->next_line;
		CURRENT_ATOM=CURRENT_LINE->first_atom;
	}
/*LAST_CHUNK is a pointer to the last allocated memory chunk from the local memory.the value of CURRENT_LINE and CURRENT_ATOM are set to the first line pointer and first atom.
*/
	LAST_CHUNK=CURRENT_ATOM;
	CURRENT_LINE=FIRST_LINE;
	CURRENT_ATOM=CURRENT_LINE->first_atom;
	return 0;
}
static int read_line_file(void)	//read a single line into the buffer from file
{
	int ch;
	int i=0;
/*reads one char from the file pointed by 'in',checks if it is EOF else reads reads one char into the buffer at a time from the file until it encounters a line feed.
*/
	ch=fgetc(in);
	if (EOF == ch)
		return 1;
	while ('\n' != ch)
	{
		CURRENT_ATOM->data[i]=(char)ch;
		i++;
		CURRENT_LINE->char_count++;
/*when the last char of the buffer of an atom is filled it calls give_atom to allocate new chunk of memory and keeps on reading until line feed is encountered.
*/
		if (MAX_SIZE == i)
		{
			CURRENT_ATOM->next_atom=give_atom(ATOM,CURRENT_ATOM,CURRENT_ATOM);
			CURRENT_ATOM=CURRENT_ATOM->next_atom;
			i=0;			
		}
		ch=fgetc(in);
	}
/*line feed character is stored in the appropriate location.
*/
	CURRENT_ATOM->data[i]=(char)ch;
	(CURRENT_LINE->char_count)++;
	return 0;
}
static int set_screen(void)
{
	initscr();
	assume_default_colors(COLOR_WHITE,COLOR_BLACK);
	raw();
	nl();
	curs_set(1);
	noecho();
	keypad(stdscr,TRUE);
	jinv_getmaxyx();
	set_export(FIRST_LINE,LAST_CHUNK,FILE_EXISTS,RWPERM);
	return 0;
}
int get_permissions(int mode)
{
	int file,perm;
	char fname[MAX_FILE_NAME];
	switch(mode)
	{
		case 1:
			return RWPERM;
		case 2://case for checking write perm for temp file in that dir.
			fname[0] = '.';
			strncpy(fname+1,FILE_NAME,MAX_FILE_NAME);
			strncat(fname,"jinvtemp",9);
			return file = open(FILE_NAME,O_WRONLY | O_CREAT);
						
	}
}
int end_buffer(int number)//the use of number shall come in case of multiple windows
{
	free(ptr);
	return 0;
}
