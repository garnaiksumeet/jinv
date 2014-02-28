#include"jinv.h"

static FILE *in; 			// input file pointer

static int save_exit_mode(void); //writes and exits or just exits or saves the file and goes to command_mode. or save new file ask for the file name(ask for either given or new)(FILE_EXISTS and FILE_NAME)
static int exit_jerk(void);	//it releases all the data structures allocated and exits the editor
static int write_file(void); //write buffer into a file
static int save_file(void);
static FILE * request_new_filename(int);
int command_mode(FILE *IN)
{
	int ch,flag_command=0;
	char command_buffer[MAX_COMMAND_LENGTH];	//the array that contains the buffer for the commands given as input

	in=IN;
	print_mesg(0,NULL);
	ch=jerk_getch();
	while ('e' != ch)
	{
		if (1 == flag_command)
			print_mesg(0,NULL);
		switch (ch)
		{
			case ':':
			{
				print_mesg(0,":");
				//jerk_flushinp();
				save_exit_mode();
				flag_command=1;
				break;
			}
			case KEY_UP:
			case 'w':
			case 'W':
			case 'j':
			case 'J':
			{
				flag_command=0;
				move_cursor(KEY_UP);
				break;
			}
			case KEY_DOWN:
			case 's':
			case 'S':
			case 'k':
			case 'K':
			{
				flag_command=0;
				move_cursor(KEY_DOWN);
				break;
			}
			case KEY_LEFT:
			case 'a':
			case 'A':
			case 'l':
			case 'L':
			{
				flag_command=0;
				move_cursor(KEY_LEFT);
				break;
			}
			case KEY_RIGHT:
			case 'd':
			case 'D':
			case 'h':
			case 'H':
			{
				flag_command=0;
				move_cursor(KEY_RIGHT);
				break;
			}
			default :
			{
				flag_command=0;
				break;
			}
		}
		ch=jerk_getch();
	}
	//print_mesg(0,NULL);
	return 0;
}
static int save_exit_mode(void)
{
	char command_buffer[MAX_COMMAND_LENGTH];

	command_reader(command_buffer,MAX_COMMAND_LENGTH);
	if (0 == strlen(command_buffer))
		return 1;
	if (('\0' == command_buffer[2]) && (0 == strncmp(command_buffer,"q!",2)))
		exit_jerk();
	if (('\0' == command_buffer[1]) && (0 == strncmp(command_buffer,"q",1)))
	{
		if (1 == set_flag_edit())
		{
			print_mesg(0,"FILE HAS BEEN EDITED,ADD ! TO OVERWRITE ALL THE CHANGES");
			return 1;
		}
		else
			exit_jerk();
	}
	if (('\0' == command_buffer[2]) && (0 == strncmp(command_buffer,"wq",2)))
	{
		save_file();
		exit_jerk();
	}
	if (('\0' == command_buffer[1]) && (0 == strncmp(command_buffer,"w",1)))
	{
		save_file();
		return 0;
	}
	else
	{
		print_mesg(0,"INVALID JINV COMMAND");
		return 1;
	}
}
static int exit_jerk(void)
{
	end_screen(1);
	end_buffer(1);
	exit(0);
}
static int write_file(void)//this shall be part of the thread that calls for writing of file to disk
{
	int i;//need to pass the entire path of the file to be created
	//tmp=fopen("r_p.txt","w");            file has already been opened
/*check for the condition if no file name is provided and ask for file name if so.
*/
	struct line_buffer *tmp_l=current_first_line();
	struct atomic_buffer *tmp_a=tmp_l->first_atom;

	while (NULL != tmp_l->next_line)
	{
		while (NULL != tmp_a)
		{
			i=0;
			while ('\0' != tmp_a->data[i])
			{
				if (MAX_SIZE == i)
					break;
				else
				{
					if (JERK_NOP == tmp_a->data[i])
					{
						i++;
						continue;
					}
					else
						fputc((int)tmp_a->data[i],in);
				}
				i++;
			}
			tmp_a=tmp_a->next_atom;
		}
		tmp_l=tmp_l->next_line;
		tmp_a=tmp_l->first_atom;
	}
	fclose(in);		//check for eof at the end of file in stdio
	//need to unlink,link and rename
	return 0;
}
static int save_file(void)//need to look for options of threading the process in case of writing the file to disk
{
	int perm=get_permissions(1);
	int writeperm;

	if ((0 == perm) || (-1 == perm))//check if got read-write permissions and then act accordingly i.e prompt for new file name in case of only read permissions and prompt for exit in case of no write permissions
	{
		if (0 == perm)
		{
			char new_file_name[MAX_FILE_NAME];

			if (0 == perm)
			{
				print_mesg(0,"READ_ONLY FILE PRESS ENTER TO ENTER A NEW FILENAME:");
			}
			else
			{
				print_mesg(0,"NO WRITE PERMISSIONS IN THIS DIRECTORY PRESS ENTER TO ENTER A NEW FILENAME");
			}
		}
		else
		{
			print_mesg(0,"NO WRITE PERMISSIONS FOR THIS FILE");
			
		}
		in=request_new_filename(1);
		if(NULL == in) return -1;
	}
	write_file();
	return 0;
}
static FILE * request_new_filename(int wait)
{
	char new_file_name[MAX_FILE_NAME];
	char response;
	FILE *out_file;
	int writeperm;
	if(1 == wait)
	{
		jerk_getch();
	}
	print_mesg(0,"NEW FILENAME:");

	command_reader(new_file_name,MAX_FILE_NAME);
	if (0 == strlen(new_file_name))
	{
		print_mesg(0,"INVALID FILENAME");
		return NULL;
	}
//check if for LINUX i.e OS syscalls

	if (0 == access(new_file_name,F_OK))
	{
		if(0 == access(new_file_name,W_OK))
		{
			print_mesg(0,"THE FILE NAME ENTERED ALREADY EXISTS: ENTER Y TO OVERWRITE N TO ENTER A NEW FILENAME");
			response = jerk_getch();
			if(('y' == response) || ('Y' == response))
			{
				return out_file=fopen(new_file_name,"w");
			}
			else
			{
				print_mesg(0,NULL);
				return request_new_filename(0);
			}
		}
		else
		{
			print_mesg(0,"WRITE PERMISSION DENIED PRESS Y TO ENTER A NEW FILENAME or N TO EXIT");
			response = jerk_getch();
			if(('y' == response) || ('Y' == response))
			{
				return request_new_filename(0);
			}
			else
			{
				exit_jerk();
			}
		}
	}
	else
	{
		writeperm = open(new_file_name,O_WRONLY | O_CREAT);
		close(writeperm);
		if(-1 == writeperm) 
		{
			print_mesg(0,"WRITE PERMISSION DENIED PRESS Y TO ENTER A NEW FILENAME or N TO EXIT");
			response = jerk_getch();
			if(('y' == response) || ('Y' == response))
			{
				return request_new_filename(0);
			}
			else
			{
				exit_jerk();
			}	
		}
		else
		{
			return out_file=fopen(new_file_name,"w");
		}
	}
	return NULL;
}
