#include"jinv.h"

static FILE *in; 			// input file pointer
static void *ptr=NULL;			//the pointer to the first location of entire chunk of memory

static int handler(void);		//handles the switching between command and edit mode.
int main(int argc,char *argv[])
{//make the use of getopt and getopt_long
	int FILE_EXISTS=0;
	char FILE_NAME[MAX_FILE_NAME];


	if (getenv ("ESCDELAY") == NULL)
		ESCDELAY = 25; //speed up ESC key reduce time to 25 milliseconds


	memset(FILE_NAME,'\0',MAX_FILE_NAME);
	if (1 == argc)
	{
		FILE_EXISTS=1;//need to call initialise with no file name
		initialise(FILE_EXISTS,(char *)FILE_NAME);	
	}
	if (2 == argc)
	{
		if (0 == strncmp("--help",argv[1],6))//require string.h
		{
			printf("Usage: jerk [OPTIONS] [FILE]\n\n");
			printf("jerk is a vi-like text mode editor\n");
			exit(1);//require stdlib.h
		}
		if (0 == strncmp("-",argv[1],1))
		{
			printf("Type: jerk --help for usage\n");
			exit(-1);
		}
		//FILE_NAME=argv[1];
		strncpy((char *)FILE_NAME,argv[1],MAX_FILE_NAME);
		initialise(FILE_EXISTS,(char *)FILE_NAME);    //takes in  filename and the flag for file existence
	}
	handler();		//handles the switching between command and edit mode.
	return -1;
}
int import_pointer(FILE *stream,void *imp_ptr)
{
	in=stream;
	ptr=imp_ptr;
	return 0;
}
int handler(void)			//handles the switching between command and edit mode.
{
	while(1)
	{
		command_mode(in);
		edit_mode();
	}
	return -1;
}
