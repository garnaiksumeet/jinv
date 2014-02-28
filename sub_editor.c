				//this file contains the sub-editor module of the editor

#include"jerk.h"


int create_buffer(void);	//creates buffer based on RWPERM and file size/existence.
int init_buffer_read(void);		//stores the input file into the buffer depending on FILE_EXISTS.
struct line_buffer *give_line(void *);	//allocates a chunk of line-structure and the first atomic structure along with the required settings of pointers from our own memory
struct atomic_buffer *give_atom(int,void *);	//allocates a chunk of atomic structure along with the required settings of pointers from our own memory
int read_line_file(void);	//reads a single line into the buffer from the file
int delete_char(void);	//deletes the character replaces with NOP, to the left of point.
int insert_char(int);	// inserts new chars into the position of point/ checks for NOP's before point or moves the remaining chars in line buffer.
int newline_insertion(void);	//when enter is pressed in edit mode
int insert_char_right(struct atomic_buffer *,int,int);	//returns 1 when inserted else 0
int insert_char_left(struct atomic_buffer *,int,int);	//returns 1 when inserted else 0

int set_buffer(void)	//allocates memory for the buffer and copies the file into buffer depending on RWPERM.
{
	create_buffer();	//creates buffer based on RWPERM and file size/existence.
	init_buffer_read();	//stores the input file into the buffer depending on FILE_EXISTS.
	CURRENT_LINE=FIRST_LINE;
	CURRENT_ATOM=CURRENT_LINE->first_atom;
}
int create_buffer(void)
{
	if (-1 == RWPERM)		//if there is no read permission on the file name then it exits
	{
		return 0;
	}
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
		FIRST_LINE->line_number=(short)1;
		FIRST_LINE->screen_line=-1;
/*its quite obvious that creating only a structure that points to line_buffer is useless until we allocate some memory for the first atom it points to,give_atom() does that job
*/
		FIRST_LINE->first_atom=give_atom(LINE,FIRST_LINE);
	}
}
struct atomic_buffer *give_atom(int jump,void *ptr)//this function allocates a memory of the size of atom from our own memory block
{
	struct atomic_buffer *tmp;
/*when jump corresponds to LINE then it is being invoked after creation of a line_buffer chunk else it is being called after the last allocation was an atom,tmp after assignment corresponds to the pointer to the atom
*/
	if (LINE == jump)
	{
		tmp=(ptr + LINE_SIZE);
	}
	else
	{
		tmp=(ptr + ATOM_SIZE);
	}
/*using memset(declared in stdlib.h) we set all the chars to NULL which will be helpful in checking for the end of that line
*/
	memset(tmp->data,'\0',MAX_SIZE);
/*CURRENT_ATOM is set to NULL(at declaration as well as in give_atom()) before this function is called after allocation of a line_buffer struct as the first atom won't have any previous atom to point.
*/
	tmp->previous_atom=CURRENT_ATOM;
	tmp->next_atom=NULL;
	CURRENT_ATOM=tmp;
	return tmp;
}
struct line_buffer *give_line(void *ptr)//this function allocates a memory of the size of line_buffer from our own memory block
{
	struct line_buffer *tmp;
/*tmp is set to the value of the pointer that points to the newly allocated block of line_buffer,the last allocated line's next_line pointer is set to tmp,line number is set and so as rest of the members of the structures appropriately.
*/
	tmp=(ptr + ATOM_SIZE);
	CURRENT_LINE->next_line=tmp;
	tmp->line_number=(CURRENT_LINE->line_number + 1);
	tmp->screen_line=-1;
	tmp->previous_line=CURRENT_LINE;
	tmp->next_line=NULL;
/*the value of CURRENT_ATOM is set to NULL as the allocation of new atom assigns the value of the new atom's previous pointer to the CURRENT_ATOM,there is a call to give_atom to allocate memory for new atom*NOTE-THIS IS THE ONLY PLACE WHERE give_atom IS CALLED WITH LINE AS AN ARGUMENT*.
CURRENT_ATOM is set to tmp and tmp returned.
*/
	CURRENT_ATOM=NULL;
	tmp->first_atom=give_atom(LINE,tmp);
	tmp->char_count=0;
	CURRENT_LINE=tmp;
	return tmp;
}
int init_buffer_read(void)		//stores the input file into the buffer depending on FILE_EXISTS.
{
/*checks for file existence and read-write permissions so as to read the file accordingly i.e if file is new/no read permission then it simply returns 1.
*/
	if (1 == FILE_EXISTS)
		return 1;
	if (-1 == RWPERM)
		return 1;
/*read_line_file returns 0 after reading one line from the file into the buffer and 1 when it encounters EOF.the below two lines of code reads an entire file into the buffer.
*/
	while (1 != read_line_file())
		give_line(CURRENT_ATOM);
/*LAST_CHUNK is a pointer to the last allocated memory chunk from the local memory.the value of CURRENT_LINE and CURRENT_ATOM are set to the first line pointer and first atom.
*/
	LAST_CHUNK=CURRENT_ATOM;
	CURRENT_LINE=FIRST_LINE;
	CURRENT_ATOM=CURRENT_LINE->first_atom;
	return 0;
}
int read_line_file(void)	//read a single line into the buffer from file
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
			CURRENT_ATOM->next_atom=give_atom(ATOM,CURRENT_ATOM);
			i=0;
		}
		ch=fgetc(in);
	}
/*line feed character is stored in the appropriate location.
*/
	CURRENT_ATOM->data[i]=(char)ch;
	CURRENT_LINE->char_count++;
	return 0;
}
int write_file(void)
{
	int i;//need to pass the entire path of the file to be created
	//tmp=fopen("r_p.txt","w");            file has already been opened
/*check for the condition if no file name is provided and ask for file name if so.
*/
	struct line_buffer *tmp_l=FIRST_LINE;
	struct atomic_buffer *tmp_a=tmp_l->first_atom;

	while (NULL != tmp_l->next_line)
	{
		while (NULL != tmp_a)
		{
			i=0;
			while ('\0' != tmp_a->data[i])
			{
				if (16 == i)
					break;
				else
				{
					if (JERK_NOP == tmp_a->data[i])
					{
						i++;
						continue;
					}
					else
						fputc((int)tmp_a->data[i],tmp_jerk);
				}
				i++;
			}
			tmp_a=tmp_a->next_atom;
		}
		tmp_l=tmp_l->next_line;
		tmp_a=tmp_l->first_atom;
	}
	fclose(tmp_jerk);		//check for eof at the end of file in stdio
	//need to unlink,link and rename
}
int edit_mode(void)
{
	int ch;

	if (0 == flag_edit)
		flag_edit=1;
	ch=jerk_getch();	//all it does is calls getch()
	while (27 != ch)		//loop until we get escape or its sequence,sequence is immaterial as we shall be flushing the rest
	{
		if ((ch > 31) && (ch < 127))
		{
			insert_char(ch);
			redisplay_line(1);
		}
		if ((ch > KEY_BREAK) && (ch < KEY_HOME))
		{
			switch (ch)
			{
				case KEY_UP:
				{
					move_cursor(KEY_UP);
					break;
				}
				case KEY_DOWN:
				{
					move_cursor(KEY_DOWN);
					break;
				}
				case KEY_LEFT:
				{
					move_cursor(KEY_LEFT);
					break;
				}
				case KEY_RIGHT:
				{
					move_cursor(KEY_RIGHT);
					break;
				}
			}
		}
		if (KEY_BACKSPACE == ch)
		{
			delete_char();
			redisplay(2);
		}
		if ('\t' == ch)
		{
			insert_char(ch);
			redisplay_line(1);
		}
		if (10 == ch)
		{
			newline_insertion();
			redisplay_line(1);
		}
		if (3 == ch)
		{
			//when we need to display to enter :q to quit
			redisplay(0);
			break;
		}
		ch=jerk_getch();
		continue;
	}
	jerk_flushinp();	//this function simply calls flushinp()
	return 0;
}
int newline_insertion(void)
{
	struct line_buffer *tmp_new_line=NULL;
	struct line_buffer *tmp_current_line=CURRENT_LINE;
	struct atomic_buffer *tmp_new_atom=NULL;
	struct atomic_buffer *tmp_current_atom=CURRENT_ATOM;
	struct line_buffer *tmp_next_line=CURRENT_LINE->next_line;
	struct line_buffer *tmp_previous_line=CURRENT_LINE->previous_line;

	give_line(LAST_CHUNK);
	LAST_CHUNK=CURRENT_ATOM;
	tmp_new_line=CURRENT_LINE;
	tmp_new_atom=CURRENT_ATOM;
	BUFFER_SCREEN.current_column=0;
	BUFFER_SCREEN.current_char_count=1;
	if ((1 == BUFFER_SCREEN.current_char_count) || (tmp_current_line->char_count == BUFFER_SCREEN.current_char_count))	//when the cursor is at first position of the line or at the last position of the line
	{
		tmp_new_line->char_count=1;
		CURRENT_ATOM->data[0]='\n';
		if (1 == BUFFER_SCREEN.current_char_count)	//when we insert line before the current line
		{
			tmp_new_line->line_number=tmp_current_line->line_number;
			tmp_new_line->next_line=tmp_new_line->previous_line;
			tmp_new_line->previous_line=tmp_previous_line;
			tmp_current_line->next_line=tmp_next_line;
			tmp_current_line->previous_line=tmp_new_line;
			CURRENT_LINE=CURRENT_LINE->next_line;
			BUFFER_SCREEN.current_index=0;
			if (1 == tmp_current_line->line_number)	//when we insert a newline before the first line of the file
				FIRST_LINE=tmp_new_line;
			else					//when line is inserted before any line other than first line
				tmp_previous_line->next_line=tmp_new_line;
			if (BUFFER_SCREEN.topline_buffer == CURRENT_LINE)
			{
				BUFFER_SCREEN.topline_buffer=(CURRENT_LINE)->previous_line;
				BUFFER_SCREEN.currentline_screen=1;
				(BUFFER_SCREEN.bottomline_buffer)->screen_line=-1;
			}
			else
			{
				if (BUFFER_SCREEN.bottomline_buffer == tmp_current_line)
				{
					(BUFFER_SCREEN.topline_buffer)->screen_line=-1;
					BUFFER_SCREEN.topline_buffer=BUFFER_SCREEN.topline_buffer->next_line;
					BUFFER_SCREEN.currentline_screen=(BUFFER_SCREEN.currentline_screen + (BUFFER_SCREEN.topline_buffer)->screen_line - 1);
				}
				else
					BUFFER_SCREEN.currentline_screen++;
			}
			//changing the line numbers
			while (NULL != tmp_current_line)
			{
				(tmp_current_line->line_number)++;
				tmp_current_line=tmp_current_line->next_line;
			}
		}
		else			//when the line inserted is after the current line
		{
			tmp_new_line->next_line=tmp_next_line;
			tmp_next_line->previous_line=tmp_new_line;
			//for finding the next char i.e find \n of the previous line
			if ((MAX_SIZE - 1) == BUFFER_SCREEN.current_index)
			{
				CURRENT_ATOM=tmp_current_atom->next_atom;
				BUFFER_SCREEN.current_index=0;
			}
			else
			{
				CURRENT_ATOM=tmp_current_atom;
				BUFFER_SCREEN.current_index++;
			}
			while (JERK_NOP == CURRENT_ATOM->data[BUFFER_SCREEN.current_index])
			{
				if ((MAX_SIZE - 1) == BUFFER_SCREEN.current_index)
				{
					CURRENT_ATOM=CURRENT_ATOM->next_atom;
					BUFFER_SCREEN.current_index=0;
				}
				else
					BUFFER_SCREEN.current_index++;
			}
			if (BUFFER_SCREEN.bottomline_buffer == CURRENT_LINE->previous_line)
			{
				(BUFFER_SCREEN.topline_buffer)->screen_line=-1;
				BUFFER_SCREEN.topline_buffer=(BUFFER_SCREEN.topline_buffer)->next_line;
				BUFFER_SCREEN.currentline_screen=(BUFFER_SCREEN.currentline_screen + (BUFFER_SCREEN.topline_buffer)->screen_line - 1);
			}
			else
			{
				(BUFFER_SCREEN.bottomline_buffer)->screen_line=-1;
				BUFFER_SCREEN.currentline_screen++;
			}
			//changing the line numbers
			while (NULL != tmp_next_line)
			{
				(tmp_next_line->line_number)++;
				tmp_next_line=tmp_next_line->next_line;
			}
		}
	}
	else	//when enter is pressed in between a line
	{
		CURRENT_LINE->next_line=tmp_next_line;
		tmp_next_line->previous_line=tmp_new_line;
		tmp_new_line->char_count=(tmp_current_line->char_count - BUFFER_SCREEN.current_char_count + 1);
		tmp_current_line->char_count=BUFFER_SCREEN.current_char_count;
		if ((MAX_SIZE - 1) == BUFFER_SCREEN.current_index) //when the current char is the last char of that atom before pressing enter
		{
			tmp_new_atom->data[0]='\n';
			tmp_new_atom->previous_atom=tmp_current_atom;
			tmp_new_line->first_atom=tmp_current_atom->next_atom;
			(tmp_current_atom->next_atom)->previous_atom=NULL;
			tmp_current_atom->next_atom=tmp_new_atom;
			CURRENT_ATOM=tmp_current_atom->next_atom;
			BUFFER_SCREEN.current_index=0;
		}
		else	//when there is still place for \n to be placed in that atom
		{
			int tmp_copy;
			int tmp_fast=(BUFFER_SCREEN.current_index + 1);

			for (tmp_copy=tmp_fast;tmp_copy < MAX_SIZE;tmp_copy++)
			{
				tmp_new_atom->data[tmp_copy - tmp_fast]=tmp_current_atom->data[tmp_copy];
				tmp_current_atom->data[tmp_copy]='\0';
			}
			tmp_current_atom->data[tmp_fast]='\n';
			for (tmp_copy=(MAX_SIZE - tmp_fast);tmp_copy < MAX_SIZE;tmp_copy++)
				tmp_new_atom->data[tmp_copy]=JERK_NOP;
			tmp_new_atom->next_atom=tmp_current_atom->next_atom;
			tmp_current_atom->next_atom=NULL;
			(tmp_new_atom->next_atom)->previous_atom=tmp_new_atom;
			BUFFER_SCREEN.current_index=tmp_fast;
			CURRENT_ATOM=tmp_current_atom;
		}
		while (NULL != tmp_next_line)
		{
			(tmp_next_line->line_number)++;
			tmp_next_line=tmp_next_line->next_line;
		}
		if (tmp_current_line == BUFFER_SCREEN.bottomline_buffer)//when the topline is to be moved out of screen
		{
			(BUFFER_SCREEN.topline_buffer)->screen_line=-1;
			BUFFER_SCREEN.topline_buffer=(BUFFER_SCREEN.topline_buffer)->next_line;	
			BUFFER_SCREEN.currentline_screen=(BUFFER_SCREEN.currentline_screen + (BUFFER_SCREEN.topline_buffer)->screen_line - 1);
		}
		else	//when the bottomline is to be moved out of screen
		{
			(BUFFER_SCREEN.bottomline_screen).screen_line=-1;
			BUFFER_SCREEN.currentline_screen++;
		}
	}
	move(BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
	screen_display();
}
int insert_char(int ch)
{
	struct atomic_buffer *tmp_insert_left=CURRENT_ATOM,*tmp_insert_right=CURRENT_ATOM;
	int tmp_index_left=BUFFER_SCREEN.current_index,tmp_index_right=BUFFER_SCREEN.current_index;
	int flag_insert=0;

	//need to take care of the case when we are actually pointing to '\n' of the previous line or NULL i.e the cursor position is first char of the first line
	if (('\n' == CURRENT_ATOM->data[BUFFER_SCREEN.current_index]) || (NULL == CURRENT_ATOM))
	{
		//checks if there is any free char in first atom and then allocates atom accordingly
		CURRENT_ATOM=CURRENT_LINE->first_atom;
		int i,j;

		for (i=0;i<MAX_SIZE;i++)
		{
			if ((CURRENT_ATOM->data[i] == JERK_NOP) || (CURRENT_ATOM->data[i] == '\0'))
				break;
		}
		if (MAX_SIZE == i)	//insert a new atom before the current atom
		{
			tmp_insert_left=give_atom(ATOM,LAST_CHUNK);
			LAST_CHUNK=CURRENT_ATOM;
			CURRENT_ATOM->next_atom=CURRENT_LINE->first_atom;
			(CURRENT_LINE->first_atom)->previous_atom=CURRENT_ATOM;
			CURRENT_LINE->first_atom=CURRENT_ATOM;
			for (i=1;i<MAX_SIZE;i++)
				CURRENT_ATOM->data[i]=JERK_NOP;
			CURRENT_ATOM->data[0]=(char)ch;
		}
		else
		{
			for (j=i;j>0;j--)
				CURRENT_ATOM->data[j]=CURRENT_ATOM->data[j - 1];
			CURRENT_ATOM->data[0]=(char)ch;
			BUFFER_SCREEN.current_index=0;
		}
		return 1;
	}
	while (1)
	{
		//for right side check
		if (1 == flag_insert)
			break;
		if ((MAX_SIZE - 1) != tmp_index_right)
			tmp_index_right++;
		else
		{
			tmp_insert_right=tmp_insert_right->next_atom;
			tmp_index_right=0;
		}
		flag_insert=insert_char_right(tmp_insert_right,tmp_index_right,ch);
		//for left side check
		if (1 == flag_insert)
			break;
		if (0 != tmp_index_left)
			tmp_index_left--;
		else
		{
			tmp_insert_left=tmp_insert_left->previous_atom;
			tmp_index_left=(MAX_SIZE - 1);
		}
		flag_insert=insert_char_left(tmp_insert_left,tmp_index_left,ch);
	}
	return 0;
}
int insert_char_right(struct atomic_buffer *tmp_atom,int tmp_index,int ch)
{
	if ((JERK_NOP == tmp_atom->data[tmp_index]) || ('\0' == tmp_atom->data[tmp_index]))
	{
		//shift all the chars after the current char upto this position and insert the new char in the position after current char position
		struct atomic_buffer *tmp_shift_atom=tmp_atom;
		struct atomic_buffer *tmp_insert_atom=CURRENT_ATOM;
		int tmp_shift_index=tmp_index;
		int tmp_insert_index=BUFFER_SCREEN.current_index;

		if ((MAX_SIZE - 1) == BUFFER_SCREEN.current_index)
		{
			tmp_insert_atom=CURRENT_ATOM->next_atom;
			tmp_insert_index=0;
		}
		while (1)		//shifts all the chars
		{
			if ((tmp_shift_atom == tmp_insert_atom) && (tmp_shift_index == tmp_insert_index))
		}
		return 1;
	}
	else
	{
		if ((MAX_SIZE - 1) == tmp_index)
		{
			if ('\n' == tmp_atom->data[tmp_index])
			{
				//need to add a new atom to the line
				return 1;
			}
		}
	}
	return 0;
}
int insert_char_left(struct atomic_buffer *tmp_atom,int tmp_index,int ch)
{
	return 0;
}
int delete_char(void)
{
	return 0;
}
