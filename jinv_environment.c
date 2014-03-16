#include "jinv.h"

static struct current_position BUFFER_SCREEN;
static int FILE_EXISTS=0; // 0 for YES 1 for NO.
static int RWPERM; //0 for read-only, 1 for Read-Write, -1 access denied
static struct line_buffer *FIRST_LINE=NULL;	//the pointer to the first line of the buffer
static struct line_buffer *CURRENT_LINE=NULL;		//the pointer to the current line in buffer
static struct atomic_buffer *CURRENT_ATOM=NULL;	//the pointer to the current atom in buffer
static struct atomic_buffer *LAST_CHUNK=NULL;		//the pointer to the last allocated chunk of memory from our local memory
static unsigned int max_column=0;			//it marks the point when the last move_left or move_right operation was performed
static int flag_edit=0;				//it sets the flag if the buffer has been edited once or not

static int screen_display(void); // displays the buffer onto the screen
static int display_line_buffer(void);		//it returns 1 when screen is more than text,-1 when it has reached the screen limit point,0 when buffer can still be written on screen
static void display_char_buffer(char);		//it displays each char in the current cursor position and also supports attributes(extensions)
static int display_atom_buffer(struct atomic_buffer *);	//displays each atom
static int delete_char(void);	//deletes the character replaces with NOP, to the left of point.
static int insert_char(int);	// inserts new chars into the position of point/ checks for NOP's before point or moves the remaining chars in line buffer.
static int newline_insertion(void);	//when enter is pressed in edit mode
static int to_super_critical(void);		//when the key press asks to move to the first char of the file
static int to_critical(void);			//when the key press asks to move to the first char of the line except the first one
static int from_critical(void);			//when the key press asks to move away from the first char of the line except the first one
static int parse_position(void);		//it parses the current_position and gives the correct location when the navigation keys are pressed up or down
static int jerk_virtual_screen(struct atomic_buffer *,int,struct atomic_buffer *,int);
static int edit_set_cursor(void);
static int check_for_refresh(int);


int jinv_getmaxyx(void)
{
	getmaxyx(stdscr,BUFFER_SCREEN.max_y,BUFFER_SCREEN.max_x);
	return 0;
}
int set_export(void *first_line,void *last_chunk,int file_exists,int rwperm)
{
	FIRST_LINE=first_line;
	CURRENT_LINE=FIRST_LINE;
	if (NULL != CURRENT_LINE)
		CURRENT_ATOM=CURRENT_LINE->first_atom;
	LAST_CHUNK=last_chunk;
	FILE_EXISTS=file_exists;
	RWPERM=rwperm;
	if (1 == FILE_EXISTS)
	{
		print_mesg((BUFFER_SCREEN.max_x/2 - 5),JINV_TRUE,"[NEW FILE]");
		//print_mesg((BUFFER_SCREEN.max_x/2 - 10),JINV_TRUE,"0L-0C");
		BUFFER_SCREEN.topline_buffer=CURRENT_LINE;
		BUFFER_SCREEN.bottomline_buffer=CURRENT_LINE;
		BUFFER_SCREEN.topline_screen=0;
		BUFFER_SCREEN.currentline_screen=0;
		BUFFER_SCREEN.bottomline_screen=0;
		BUFFER_SCREEN.current_column=0;
		BUFFER_SCREEN.current_char_count=1;//in a new file we insert an endline char in the first line
		BUFFER_SCREEN.current_index=-1;
		CURRENT_ATOM=NULL;
		CURRENT_LINE->screen_line=0;		//need to check when in case of newfile
		refresh();
		move(0,0);
		return 0;
	}
	if (-1 == RWPERM)
	{
		print_mesg((BUFFER_SCREEN.max_x/2 - 10),JINV_TRUE,"[PERMISSION DENIED]");
		curs_set(0);
		refresh();
		return 1;
	}
	if ((1 == RWPERM) || (0 == RWPERM))
	{
		BUFFER_SCREEN.topline_buffer=CURRENT_LINE;
		BUFFER_SCREEN.topline_screen=0;
		BUFFER_SCREEN.currentline_screen=0;
		BUFFER_SCREEN.current_column=0;
		BUFFER_SCREEN.current_char_count=1;
		BUFFER_SCREEN.current_index=-1;
		CURRENT_ATOM=NULL;
		move(0,0);
		screen_display();
		return 0;
	}
	return -1;
}
static int screen_display(void)	//before calling screen_display we must set topline_buffer,currentline_screen,current_column, current_char_count
{
	//int i=0;
	int c_x,c_y,tmp_useless;

	getyx(stdscr,c_y,c_x);
	getmaxyx(stdscr,BUFFER_SCREEN.max_y,BUFFER_SCREEN.max_x);
	BUFFER_SCREEN.bottomline_buffer=BUFFER_SCREEN.topline_buffer;
	(BUFFER_SCREEN.topline_buffer)->screen_line=0;
	//mvaddnstr(SCREEN_STATUS,3,FILE_NAME,MAX_FILE_NAME);
	//mvaddnstr(SCREEN_STATUS,(BUFFER_SCREEN.max_x/2 - 3),"C-MODE",10);
	//mvprintw(SCREEN_STATUS,(BUFFER_SCREEN.max_x - 6),"%dL-%dC",(BUFFER_SCREEN.currentline_screen+1),BUFFER_SCREEN.current_char_count);
	move(0,0);
	BUFFER_SCREEN.bottomline_screen=BUFFER_SCREEN.currentline_screen;
	getyx(stdscr,BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
	while (1)
	{
		display_line_buffer();
		BUFFER_SCREEN.bottomline_screen=BUFFER_SCREEN.currentline_screen;
		if ((1 == (SCREEN_STATUS - BUFFER_SCREEN.bottomline_screen)) || (0 == (SCREEN_STATUS - BUFFER_SCREEN.bottomline_screen)))
		{
			if (0 == (SCREEN_STATUS - BUFFER_SCREEN.bottomline_screen))
			{
				BUFFER_SCREEN.currentline_screen=c_y;
				move(c_y,c_x);
				refresh();
				return 0;
			}
			else
			{
				BUFFER_SCREEN.bottomline_buffer=BUFFER_SCREEN.bottomline_buffer->next_line;
				if (NULL == BUFFER_SCREEN.bottomline_buffer->next_line)
				{
					BUFFER_SCREEN.bottomline_buffer=BUFFER_SCREEN.bottomline_buffer->previous_line;
					BUFFER_SCREEN.currentline_screen=c_y;
					move(c_y,c_x);
					refresh();
					return 0;
				}
				else
				{//scope for better codes to check for better options to check for exact display of chars
					if (BUFFER_SCREEN.bottomline_buffer->char_count < (int)(BUFFER_SCREEN.max_x-  (float)BUFFER_SCREEN.max_x/5.0))//use the virtual screen function for checking the fitting of the line(NOTE HERE)
					{
						getyx(stdscr,(BUFFER_SCREEN.bottomline_buffer)->screen_line,tmp_useless);
						display_line_buffer();
					}
					else
					{
						addch('@');
						clrtoeol();
						(BUFFER_SCREEN.bottomline_buffer)->screen_line=-1;
						BUFFER_SCREEN.bottomline_buffer=BUFFER_SCREEN.bottomline_buffer->previous_line;
					}
					BUFFER_SCREEN.currentline_screen=c_y;
					move(c_y,c_x);
					refresh();
					return 1;
				}
			}
		}
		BUFFER_SCREEN.bottomline_buffer=BUFFER_SCREEN.bottomline_buffer->next_line;
		getyx(stdscr,(BUFFER_SCREEN.bottomline_buffer)->screen_line,tmp_useless);
		if (NULL == BUFFER_SCREEN.bottomline_buffer->next_line)
		{
			BUFFER_SCREEN.bottomline_buffer=BUFFER_SCREEN.bottomline_buffer->previous_line;
			BUFFER_SCREEN.currentline_screen=c_y;
			move(c_y,c_x);
			refresh();
			return 0;
		}
	}
}
static int display_line_buffer(void)		//before calling we must set bottomline_buffer to CURRENT_LINE when refreshing individual line and return bottomline_buffer its original value after the call
{
	struct atomic_buffer *tmp;

	tmp=BUFFER_SCREEN.bottomline_buffer->first_atom;
	while (NULL != tmp)
	{
		display_atom_buffer(tmp);
		tmp=tmp->next_atom;
	}
	getyx(stdscr,BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
	return 0;
}
static int display_atom_buffer(struct atomic_buffer *cur_atom)
{
	int i=0;

	if (('\0' == cur_atom->data[MAX_SIZE - 1]) || ('\n' == cur_atom->data[MAX_SIZE - 1]))
	{
		while ('\n' != cur_atom->data[i])
		{
			if (JERK_NOP == cur_atom->data[i])
			{
				i++;
				continue;
			}
			else
			{
				//addch((const chtype)cur_atom->data[i]);
				display_char_buffer(cur_atom->data[i]);
				i++;
			}
		}
		//addch((const chtype)cur_atom->data[i]);
		display_char_buffer(cur_atom->data[i]);
		return 1;
	}
	else
	{
		for (i=0;i<MAX_SIZE;i++)
		{
			if (JERK_NOP == cur_atom->data[i])
				continue;
			else
				//addch((const chtype)cur_atom->data[i]);
				display_char_buffer(cur_atom->data[i]);
		}
	}
	return 0;
}
void display_char_buffer(char ch)
{
	addch((const chtype)ch);
}
int edit_mode(void)
{
	int ch,return_code=0;

	if (0 == RWPERM)//need to allow to edit if has permissions to create a file in that directory
	{
		print_mesg(0,JINV_TRUE,"[READ-ONLY FILE]");
		return 1;
	}
	if (-1 == RWPERM)
	{
		print_mesg(0,JINV_TRUE,"[NO PERMISSION TO READ THE FILE]");
		return 1;
	}
	if (0 == flag_edit)
		flag_edit=1;
	print_mesg(0,JINV_TRUE,"--EDIT-MODE--");
	ch=getch();
	while (27 != ch)		//loop until we get escape or its sequence,sequence is immaterial as we shall be flushing the rest
	{
		if ((ch > 31) && (ch < 127))
		{
			return_code = insert_char(ch);
			redisplay_line(return_code);
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
			return_code=delete_char();
			redisplay_line(return_code);
		}
		if ('\t' == ch)
		{
			return_code = insert_char(ch);
			redisplay_line(return_code);
		}
		if (10 == ch)
		{
			newline_insertion();
			redisplay_line(1);
		}
		if (3 == ch)
		{
			print_mesg(0,JINV_TRUE,"ENTER :q to exit the editor");
			redisplay_line(0);
			break;
		}
		ch=getch();
		//continue;
	}
	print_mesg(0,JINV_TRUE,NULL);
	//flushinp();
	return 0;
}
static int newline_insertion(void)
{
	struct line_buffer *tmp_new_line=NULL;
	struct line_buffer *tmp_current_line=CURRENT_LINE;
	struct atomic_buffer *tmp_new_atom=NULL;
	struct atomic_buffer *tmp_current_atom=CURRENT_ATOM;
	struct line_buffer *tmp_next_line=CURRENT_LINE->next_line;
	struct line_buffer *tmp_previous_line=CURRENT_LINE->previous_line;

	//these do the basic job of allocating and assigning
	tmp_new_line=give_line(LAST_CHUNK,CURRENT_LINE);
	LAST_CHUNK=tmp_new_line->first_atom;
	tmp_new_atom=tmp_new_line->first_atom;
	tmp_new_line->screen_line=CURRENT_LINE->screen_line;//DONOT CHANGE THIS LINE.assigned to the same line number as the current line
	if ((1 == BUFFER_SCREEN.current_char_count) || (CURRENT_LINE->char_count == BUFFER_SCREEN.current_char_count))	//when the cursor is at first position of the line or at the last position of the line
	{
		tmp_new_line->char_count=1;
		tmp_new_atom->data[0]='\n';
		if (1 == BUFFER_SCREEN.current_char_count)	//when we insert line before the current line then the new cursor shall remain below the same char and in buffer we are to point to the \n of the line just inserted
		{
			//node insertion before CURRENT_LINE and change of line number
			(tmp_new_line->line_number)--;
			tmp_new_line->next_line=CURRENT_LINE;
			tmp_new_line->previous_line=CURRENT_LINE->previous_line;
			CURRENT_LINE->previous_line=tmp_new_line;
			CURRENT_ATOM=tmp_new_line->first_atom;
			BUFFER_SCREEN.current_index=0;
			if (1 == CURRENT_LINE->line_number)	//when we insert a newline before the first line of the file
				FIRST_LINE=tmp_new_line;
			else					//when line is inserted before any line other than first line
				(CURRENT_LINE->previous_line)->next_line=tmp_new_line;
			//if case is when we are at the first position of the first line of the screen
			if (BUFFER_SCREEN.topline_buffer == CURRENT_LINE)
				screen_refresh_set(SCROLL_DOWN,NULL);
			else
			{
				if (BUFFER_SCREEN.bottomline_buffer == CURRENT_LINE)//when we are the first position of the last line of the screen and we press enter then we make the screen scroll up
					screen_refresh_set(SCROLL_UP,NULL);
				else
					screen_refresh_set(SCROLL_PARTIAL,CURRENT_LINE->first_atom);
			}
			//don't need to change the value of CURRENT_LINE
		}
		else			//when the line inserted is after the current line i.e we press enter when at the end of any line
		{
			//on the screen the cursor shall be on the new line just inserted and in buffer we shall be pointing to the \n of the previous line(CURRENT_LINE before pressing enter)
			//node insertion after the CURRENT_LINE and changing of CURRENT_LINE and CURRENT_ATOM and current_index in BUFFER_SCREEN
			tmp_new_line->next_line=CURRENT_LINE->next_line;
			tmp_new_line->previous_line=CURRENT_LINE;
			(CURRENT_LINE->next_line)->previous_line=tmp_new_line;
			CURRENT_LINE->next_line=tmp_new_line;
			//we now need to point to the \n of the CURRENT_LINE(the old one before pressing enter)
			//we donot search for last atom and then for \n in it because we know that it is right now pointing to the char just before \n.We are doing this for optimisation instead of calling to_critical
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
			//when we press enter at the last line the screen scrolls up but in any other case it scrolls down partially
			if (BUFFER_SCREEN.bottomline_buffer == CURRENT_LINE)//the CURRENT_LINE is currently the old one
				screen_refresh_set(SCROLL_UP,NULL);
			else
				screen_refresh_set(SCROLL_PARTIAL,tmp_new_line->first_atom);
			CURRENT_LINE=tmp_new_line;
			tmp_current_line=tmp_new_line;	//this assignment is just to make up for the laziness of not writing 5 more line because we are to increase lines from next line but in the above case from current line
		}
		while (NULL != tmp_current_line)//update the new line numbers
		{
			(tmp_current_line->line_number)++;
			tmp_current_line=tmp_current_line->next_line;
		}
	}
	else	//when enter is pressed in between a line
	{
		tmp_new_line->next_line=CURRENT_LINE->next_line;
		tmp_new_line->previous_line=CURRENT_LINE;
		(CURRENT_LINE->next_line)->previous_line=tmp_new_line;
		CURRENT_LINE->next_line=tmp_new_line;
		tmp_new_line->char_count=(CURRENT_LINE->char_count - BUFFER_SCREEN.current_char_count + 1);//y-(x+1)+1
		CURRENT_LINE->char_count=BUFFER_SCREEN.current_char_count;//(x+1)
		BUFFER_SCREEN.current_char_count=1;
		if ((MAX_SIZE - 1) == BUFFER_SCREEN.current_index) //when the current char is the last char of that atom before pressing enter
		{
			tmp_new_atom->data[0]='\n';
			tmp_new_atom->previous_atom=CURRENT_ATOM;
			tmp_new_line->first_atom=CURRENT_LINE->first_atom;
			(CURRENT_ATOM->next_atom)->previous_atom=NULL;
			CURRENT_ATOM->next_atom=tmp_new_atom;
			CURRENT_ATOM=tmp_new_atom;
			BUFFER_SCREEN.current_index=0;
		}
		else	//when there is still place for \n to be placed in that atom
		{
			int tmp_copy;
//SEE FROM HERE
			for (tmp_copy=(BUFFER_SCREEN.current_index + 1);tmp_copy < MAX_SIZE;tmp_copy++)
			{
				tmp_new_atom->data[tmp_copy - (BUFFER_SCREEN.current_index + 1)]=tmp_current_atom->data[tmp_copy];
				tmp_current_atom->data[tmp_copy]='\0';
			}
			tmp_current_atom->data[(BUFFER_SCREEN.current_index + 1)]='\n';
			if (NULL == tmp_current_atom->next_atom)	//when there is no atom after this atom
				tmp_new_atom->data[MAX_SIZE - (BUFFER_SCREEN.current_index + 1)]='\n';
			else
			{
				for (tmp_copy=(MAX_SIZE - (BUFFER_SCREEN.current_index + 1));tmp_copy < MAX_SIZE;tmp_copy++)
					tmp_new_atom->data[tmp_copy]=JERK_NOP;
				tmp_new_atom->next_atom=tmp_current_atom->next_atom;
				tmp_current_atom->next_atom=NULL;
				(tmp_new_atom->next_atom)->previous_atom=tmp_new_atom;
			}
			BUFFER_SCREEN.current_index++;
			CURRENT_ATOM=tmp_current_atom;
		}
		if (BUFFER_SCREEN.bottomline_buffer == CURRENT_LINE)//the CURRENT_LINE is currently the old one
			screen_refresh_set(SCROLL_UP,NULL);
		else
			screen_refresh_set(SCROLL_PARTIAL,tmp_new_line->first_atom);
		CURRENT_LINE=tmp_new_line;
		while (NULL != tmp_next_line)
		{
			(tmp_next_line->line_number)++;
			tmp_next_line=tmp_next_line->next_line;
		}
	}
	max_column=0;
	return 1;
}
int screen_refresh_set(int scroll_opt,void *tmp_first_atom)//this function is to be called when the CURRENT_LINE is not updated after editing but the screen is refreshed
{
	switch (scroll_opt)
	{
		case SCROLL_UP:
		{
			//when the screen is scrolled up one line
			(BUFFER_SCREEN.topline_buffer)->screen_line=-1;
			BUFFER_SCREEN.topline_buffer=(BUFFER_SCREEN.topline_buffer)->next_line;
			BUFFER_SCREEN.currentline_screen=(BUFFER_SCREEN.currentline_screen + ((BUFFER_SCREEN.topline_buffer)->screen_line - 1));//since topline_buffer is now the line next to topline.
			BUFFER_SCREEN.current_column=1;
			break;
		}
		case SCROLL_DOWN:
		{
			//when the screen is scrolled down one line
			BUFFER_SCREEN.topline_buffer=(BUFFER_SCREEN.topline_buffer)->previous_line;
			BUFFER_SCREEN.currentline_screen=1;
			BUFFER_SCREEN.current_column=1;
			(BUFFER_SCREEN.bottomline_buffer)->screen_line=-1;
			break;
		}
		case SCROLL_PARTIAL:
		{
			//when we are changing the screen but not actually scrolling
			BUFFER_SCREEN.currentline_screen++;
			if (BUFFER_SCREEN.max_x == (BUFFER_SCREEN.currentline_screen + 2))
			{
				struct atomic_buffer *tmp_atom_virtual=tmp_first_atom;
				int tmp_line_wrap,tmp_end_index=0;

				while (NULL != tmp_atom_virtual->next_atom)
					tmp_atom_virtual=tmp_atom_virtual->next_atom;
				while (tmp_atom_virtual->data[tmp_end_index] != '\n')
					tmp_end_index++;
				tmp_line_wrap=(jerk_virtual_screen(tmp_first_atom,0,tmp_atom_virtual,tmp_end_index)/10000 + 1);//number of lines its wrapped around
				if (tmp_line_wrap > 1)
				{
					struct line_buffer *tmp_topline=BUFFER_SCREEN.topline_buffer;
					//to check to find the number of lines required to shift above to fit the line
					do
					{
						tmp_topline=tmp_topline->next_line;
					}while ((tmp_line_wrap - 1) >= (tmp_topline->screen_line - (BUFFER_SCREEN.topline_buffer)->screen_line));
					while (BUFFER_SCREEN.topline_buffer != tmp_topline)
					{
						(BUFFER_SCREEN.topline_buffer)->screen_line=-1;
						BUFFER_SCREEN.topline_buffer=(BUFFER_SCREEN.topline_buffer)->next_line;
					}
				}
			}
			(BUFFER_SCREEN.bottomline_buffer)->screen_line=-1;
			BUFFER_SCREEN.bottomline_buffer=((BUFFER_SCREEN.bottomline_buffer)->next_line);
			BUFFER_SCREEN.current_column=1;
			break;
		}
	}
	return 0;
}
static int insert_char(int ch)
{
	int wrapped;
	struct atomic_buffer *temp_cp,*traverse;
	wrapped = (CURRENT_LINE->next_line->screen_line - CURRENT_LINE->screen_line);
	struct atomic_buffer *tmp_insert=CURRENT_ATOM;
	int tmp_index=BUFFER_SCREEN.current_index;
	int flag_insert=0;
	int flag_critical=0;
	//need to take care of the case when we are actually pointing to '\n' of the previous line or NULL i.e the cursor position is first char of the first line
	if (NULL == CURRENT_ATOM)
	{
		CURRENT_ATOM=CURRENT_LINE->first_atom;
		flag_critical = 1;
	}

	if('\n' == CURRENT_ATOM->data[BUFFER_SCREEN.current_index])
	{
		CURRENT_ATOM=CURRENT_LINE->first_atom;
		flag_critical = 1;
	}
 
	if (1 == flag_critical)
	{
		//checks if there is any free char in first atom and then allocates atom accordingly
		int i,j;
		char temp;
		for (i=0;i<MAX_SIZE;i++)
		{
			temp = CURRENT_ATOM->data[i];
			if ((temp == JERK_NOP) || (temp == '\0'))
				break;
		}
		if (MAX_SIZE == i)	//insert a new atom before the current atom if no space in current atom
		{
			tmp_insert=give_atom(ATOM,LAST_CHUNK,CURRENT_ATOM);
			LAST_CHUNK=tmp_insert;
			for (i=1;i<MAX_SIZE;i++)
				tmp_insert->data[i]=JERK_NOP;
			tmp_insert->data[0]=(char)ch;
			BUFFER_SCREEN.current_index=0;
			CURRENT_ATOM->previous_atom = tmp_insert;
			tmp_insert->next_atom = CURRENT_ATOM;
			tmp_insert->previous_atom = NULL;
			CURRENT_LINE->first_atom = tmp_insert;
			CURRENT_ATOM = tmp_insert;
		}
		else
		{
			for (j=i;j>=0;j--)
				CURRENT_ATOM->data[j]=CURRENT_ATOM->data[j - 1];
			CURRENT_ATOM->data[0]=(char)ch;
			BUFFER_SCREEN.current_index=0;
		}
		CURRENT_LINE->char_count++;
		BUFFER_SCREEN.current_char_count++;
		return check_for_refresh(wrapped);
	}
	else
	{
		int i,n,index=BUFFER_SCREEN.current_index;
		char tmpch;
		for(i=index;i<MAX_SIZE;i++)
		{
			tmpch = CURRENT_ATOM->data[i];
			if('\0' == tmpch || JERK_NOP ==  tmpch)
				break;
		}
		if(MAX_SIZE == i)
		{
			tmp_insert=give_atom(ATOM,LAST_CHUNK,CURRENT_ATOM);
			LAST_CHUNK=tmp_insert;
			temp_cp = CURRENT_ATOM->next_atom;
			CURRENT_ATOM->next_atom = tmp_insert;
			tmp_insert->previous_atom = CURRENT_ATOM;
			tmp_insert->next_atom = temp_cp;
			for(i=0;i<MAX_SIZE;i++) 
				tmp_insert->data[i] = JERK_NOP;
			for(i = index+1;i<MAX_SIZE;i++)
			{
				tmp_insert->data[i-(index+1)] = CURRENT_ATOM->data[i];
				CURRENT_ATOM->data[i] = JERK_NOP;
			}
			if(MAX_SIZE == (index + 1))
			{
				index = -1;
				CURRENT_ATOM = tmp_insert;
			}
			CURRENT_ATOM->data[index + 1] = (char)ch;
		}
		else
		{
			for(i;i>index;i--)
			{
				CURRENT_ATOM->data[i] = CURRENT_ATOM->data[i - 1];	
			}
			CURRENT_ATOM->data[index + 1] = (char)ch;
		}
		CURRENT_LINE->char_count++;
		BUFFER_SCREEN.current_char_count++;
		BUFFER_SCREEN.current_index++;
		if(MAX_SIZE == BUFFER_SCREEN.current_index)
			BUFFER_SCREEN.current_index = 0;
		return check_for_refresh(wrapped);	
	}
	return 0;
}
static int check_for_refresh(int wrapped)
{
		struct atomic_buffer *traverse;
		int i,n,will_wrap;
		traverse = CURRENT_LINE->first_atom;
		while(NULL != traverse->next_atom) traverse = traverse->next_atom;
		for(i=0;i<MAX_SIZE;i++)
		{
			if('\n' == traverse->data[i])
				break;
		}
		will_wrap = jerk_virtual_screen(CURRENT_LINE->first_atom,0,traverse,i-1);
		n = will_wrap%10000;//jerk_virtual_screen gives both no of wrapped lines and virtual column
		max_column = (n-1)*BUFFER_SCREEN.max_x + BUFFER_SCREEN.current_column;//change the index in virtual screen as well	
		will_wrap = will_wrap/10000 + 1;//get no of wrapped lines.   I added +1 as virtual screen returns (line-1 * 10000 + column)
		if(will_wrap == wrapped) //don't need to refresh the entire screen just the current line
		{

			return 2;
		}
		else//refresh entire screen
		{
			return 1;
		}
}
static int delete_char(void)
{
	struct line_buffer *temp_line,*temp_topline_buffer=NULL;
	struct atomic_buffer *temp_atom,*pointer_atom; //pointer for current char in display
	int pointer_char_index,loop,temp_atom_index=0;
	if(NULL == CURRENT_ATOM)//super critical
	{
		return 0;
	}
	else if(1 == BUFFER_SCREEN.current_char_count) //move up current char is the first char in the line so shift the current line upstairs.
	{
		temp_line  = CURRENT_LINE->previous_line;
		temp_atom = temp_line->first_atom;
		//Line deletion when only '\n' present
		if(1 == CURRENT_LINE->char_count) //line has only a '\n'
		{
			temp_line->next_line = CURRENT_LINE->next_line; //delete the current line. will change the current line nos afterwards
			CURRENT_LINE->next_line->previous_line  = temp_line;
			CURRENT_LINE->previous_line = NULL;
			CURRENT_LINE->next_line = NULL;
			CURRENT_LINE = temp_line;
			while(NULL != temp_atom->next_atom)//goto the last atom for finding '\n'
				temp_atom = temp_atom->next_atom;
			CURRENT_ATOM = temp_atom;
			BUFFER_SCREEN.current_char_count = CURRENT_LINE->char_count;
			max_column = CURRENT_LINE->char_count - 1;
			while('\n' != temp_atom->data[temp_atom_index])
				temp_atom_index++;
			if (1 == CURRENT_LINE->char_count)
			{
				//if the new currentline also contains one char
				if (1 == CURRENT_LINE->line_number)
					to_super_critical();
				else
					to_critical();
			}
			else
			{
				//find the last printable char of the new current line
				if (0 == temp_atom_index)
				{
					temp_atom=temp_atom->previous_atom;
					temp_atom_index=(MAX_SIZE - 1);
				}
				else
					temp_atom_index--;
				while (JERK_NOP == temp_atom->data[temp_atom_index])
				{
					if (0 == temp_atom_index)
					{
						temp_atom=temp_atom->previous_atom;
						temp_atom_index=(MAX_SIZE - 1);
					}
					else
						temp_atom_index--;
				}
				BUFFER_SCREEN.current_index = temp_atom_index;
			}
		}
		else
		{
			if (1 == temp_line->char_count)//if previous line has only \n
			{
				//change the topline buffer accordingly
				if (temp_line == BUFFER_SCREEN.topline_buffer->previous_line)	//to set the value of a local variable such as to check for screen redisplay or not
					temp_topline_buffer=temp_line;
				if (temp_line == BUFFER_SCREEN.topline_buffer)
					BUFFER_SCREEN.topline_buffer=CURRENT_LINE;
				if(1 != temp_line->line_number) //considering temp_line == first line
				{
					temp_line->previous_line->next_line=CURRENT_LINE;
				}
				else
				{
					FIRST_LINE=CURRENT_LINE;
					BUFFER_SCREEN.topline_buffer=CURRENT_LINE;
				}
				CURRENT_LINE->previous_line=temp_line->previous_line;
				temp_line->previous_line=NULL;
				temp_line->next_line=NULL;
				CURRENT_LINE->line_number--;
				max_column = 0;
				if (1 == CURRENT_LINE->line_number)
					to_super_critical();
				else
					to_critical();
			}
			else
			{
				while(NULL != temp_atom->next_atom)//goto the last atom for finding '\n'
					temp_atom = temp_atom->next_atom;
				while('\n' != temp_atom->data[temp_atom_index])
					temp_atom_index++;
				for(loop = temp_atom_index;loop< MAX_SIZE;loop++)//insert NOP's in and after '\n'
					temp_atom->data[loop] = JERK_NOP;
				//find the char which is not nop on the left side of current so as to point to it in display and other modules.
				//finding the current char on left
				if(0 == temp_atom_index)
				{
					pointer_atom = temp_atom->previous_atom;
					pointer_char_index = MAX_SIZE - 1;
				}
				else
				{
					pointer_atom = temp_atom;
					pointer_char_index = temp_atom_index - 1;
				}
				//searching for char on left side of '\n'
				while(JERK_NOP == pointer_atom->data[pointer_char_index])
				{
					if(0 == pointer_char_index) //goto previous atom no valid char found on this atom except '\n'
					{
						pointer_atom=pointer_atom->previous_atom;
						pointer_char_index=(MAX_SIZE - 1);
					}
					else
						pointer_char_index--;
				}
				//starting shift
				BUFFER_SCREEN.current_char_count = temp_line->char_count;
				max_column = temp_line->char_count - 1;
				temp_line->char_count = CURRENT_LINE->char_count + temp_line->char_count - 1; //-1 bcoz we just have one \n instead of two
				CURRENT_ATOM = pointer_atom;
				BUFFER_SCREEN.current_index = pointer_char_index;
				temp_atom->next_atom = CURRENT_LINE->first_atom;
				temp_atom->next_atom->previous_atom=temp_atom;
				temp_line->next_line = CURRENT_LINE->next_line;
				temp_line->next_line->previous_line = temp_line;
				CURRENT_LINE->next_line=NULL;
				CURRENT_LINE->previous_line=NULL;
				CURRENT_LINE = temp_line;
			}
		}
		//decrease line
		temp_line = CURRENT_LINE->next_line;
		while(NULL != temp_line->next_line)
		{
			(temp_line->line_number)--;
			temp_line = temp_line->next_line;
		}
		if (NULL == temp_topline_buffer)	//this signifies that there is need to change the current screen
			return 1;	//when there is deletion of line structure
		else
			return 0;
	}
	else
	{
		temp_atom_index  = BUFFER_SCREEN.current_index;
		temp_atom = CURRENT_ATOM;
		temp_atom->data[temp_atom_index] = JERK_NOP;
		
		//for getting the current char on left.
		if(2 == BUFFER_SCREEN.current_char_count)//when going to critical state after deleting have to point to '\n'
		{
			(CURRENT_LINE->char_count)--;
			max_column = 0;

			if (1 == CURRENT_LINE->line_number)
				to_super_critical();
			else
				to_critical();
		}
		else
		{
			while(JERK_NOP == temp_atom->data[temp_atom_index])
			{
				if(0 == temp_atom_index)
				{
					temp_atom = temp_atom->previous_atom;
					temp_atom_index = MAX_SIZE - 1;
				}
				else
					temp_atom_index--;
			}
			(CURRENT_LINE->char_count)--;
			CURRENT_ATOM = temp_atom;
			BUFFER_SCREEN.current_index = temp_atom_index;
			BUFFER_SCREEN.current_char_count--;
			max_column--;
		}
	}
	return 2;	//when there is insertion of just NOP in place of that char
}
int jerk_getch(void)
{
	return getch();
}
void jerk_flushinp(void)
{
	flushinp();
}
static int edit_set_cursor(void)
{
	if (NULL == CURRENT_ATOM)	//why i m writing this as a separate case cause it would make trouble if the CURRENT_ATOM is NULL and we try to access any thing from it in the successive lines
	{
		BUFFER_SCREEN.currentline_screen=0;
		BUFFER_SCREEN.current_column=0;
		max_column=0;
	}
	else if ('\n' == CURRENT_ATOM->data[BUFFER_SCREEN.current_index])
	{
		BUFFER_SCREEN.currentline_screen=CURRENT_LINE->screen_line;
		BUFFER_SCREEN.current_column=0;
		max_column=0;
	}
	else
	{
		int line_column;
		line_column=jerk_virtual_screen(CURRENT_LINE->first_atom,0,CURRENT_ATOM,BUFFER_SCREEN.current_index);
		BUFFER_SCREEN.currentline_screen=(CURRENT_LINE->screen_line + (line_column/10000));
		BUFFER_SCREEN.current_column=(line_column % 10000);
		max_column=((BUFFER_SCREEN.max_x - 1)*(line_column/10000) + BUFFER_SCREEN.current_column);
	}
	move(BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
}
int redisplay_line(int opt)
{
	struct line_buffer *tmp_display_line=NULL;
	switch (opt)
	{
		case 0://simply return i.e when there is nothing to be done
		{
			return 0;
		}
		case 1://when we need to redisplay the entire screen
		{
			if (-1 == CURRENT_LINE->screen_line)
			{
				tmp_display_line=CURRENT_LINE->next_line;
				if (-1 == tmp_display_line->screen_line)	//this gives us the knowledge regarding the scrolling of screen
				{
					//for insertion case because deletion cannot have scrolling downwards
					//scroll down
					
					move(BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
				}
				else
				{
					BUFFER_SCREEN.topline_buffer=CURRENT_LINE;
					move(0,0);
					screen_display();
				}
			}
			else
			{
				move(0,0);
				screen_display();
				tmp_display_line=BUFFER_SCREEN.topline_buffer->next_line;
				while (-1 == CURRENT_LINE->screen_line)//this is to make sure that we are able to make the changes reflect even when we are editing in the last line
				{
					(BUFFER_SCREEN.topline_buffer)->screen_line=-1;
					BUFFER_SCREEN.topline_buffer=tmp_display_line;
					tmp_display_line=tmp_display_line->next_line;
					move(0,0);
					screen_display();
				}
			}
			//set the screen-line and screen-column and set max_column
			edit_set_cursor();
			break;
		}
		case 2:
		{
			//just need to redisplay the line along with considering the case for when line increases decreases in case of wraping
			struct line_buffer *tmp_bottomline_buffer=BUFFER_SCREEN.bottomline_buffer;
			int tmp_x,tmp_y;

			BUFFER_SCREEN.bottomline_buffer=CURRENT_LINE;
			tmp_display_line=CURRENT_LINE->next_line;
			move(CURRENT_LINE->screen_line,0);
			display_line_buffer();
			refresh();
			getyx(stdscr,tmp_y,tmp_x);
			if (tmp_y == tmp_display_line->screen_line)	//when we need to redisplay only the line
			{
				edit_set_cursor();
				BUFFER_SCREEN.bottomline_buffer=tmp_bottomline_buffer;
			}
			else
				redisplay_line(1);
			break;
		}
	}
	return 0;
}
int move_cursor(int key_val)
{
	int c_x,c_y,tmp_line_move=0;

	getyx(stdscr,c_y,c_x);
	switch (key_val)
	{
		case KEY_UP:
		{
			if (1 == CURRENT_LINE->line_number)
				return 1;
			else
			{
				CURRENT_LINE=CURRENT_LINE->previous_line;
				if ((1 == BUFFER_SCREEN.current_char_count) && (0 == max_column))//when we are actually pointing at \n of previous line
				{
					BUFFER_SCREEN.current_column=0;
					if (1 == CURRENT_LINE->line_number)
						to_super_critical();
					else
						to_critical();
				}
				else
					tmp_line_move=parse_position();//parse position shall take care in case we get \n as the only char in that up/down line(we need to care about the line being moved to is 1 or not)
				if (BUFFER_SCREEN.topline_buffer == CURRENT_LINE->next_line)
				{
					int tmp_current_column_up=BUFFER_SCREEN.current_column;
					BUFFER_SCREEN.topline_buffer=CURRENT_LINE;
					BUFFER_SCREEN.currentline_screen=tmp_line_move;
					screen_display();
					BUFFER_SCREEN.current_column=tmp_current_column_up;

					struct line_buffer *tmp_bottomline=BUFFER_SCREEN.bottomline_buffer->next_line;
					while (-1 != tmp_bottomline->screen_line)
					{
						tmp_bottomline->screen_line=-1;
						tmp_bottomline=tmp_bottomline->next_line;
					}
				}
				else
					BUFFER_SCREEN.currentline_screen=(CURRENT_LINE->screen_line + tmp_line_move);
				//mvprintw(SCREEN_STATUS,0,"Index-%d\tData_int-%d\tChar-%c\tMax Column-%d",BUFFER_SCREEN.current_index,(int)CURRENT_ATOM->data[BUFFER_SCREEN.current_index],CURRENT_ATOM->data[BUFFER_SCREEN.current_index],max_column);
				move(BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
			}
			break;
		}
		case KEY_DOWN:
		{
			struct line_buffer *tmp_last;
			tmp_last=CURRENT_LINE->next_line;
			if (NULL == tmp_last->next_line)
				return 1;
			else
			{
				CURRENT_LINE=tmp_last;
				if ((1 == BUFFER_SCREEN.current_char_count) && (0 == max_column))//when we are actually pointing to \n of previous line
				{
					to_critical();
					BUFFER_SCREEN.current_column=0;
				}
				else
					tmp_line_move=parse_position();
				if (BUFFER_SCREEN.bottomline_buffer == CURRENT_LINE->previous_line)	//the bug was when one line moved up in screen but more than 1 line comes from the bottom
				{
					int tmp_current_column_down=BUFFER_SCREEN.current_column;
					while (-1 == CURRENT_LINE->screen_line)		//the idea of looping is until we get the current-line in the screen we keep on refreshing the screen
					{
						(BUFFER_SCREEN.topline_buffer)->screen_line=-1;
						BUFFER_SCREEN.topline_buffer=BUFFER_SCREEN.topline_buffer->next_line;
						screen_display();
					}
					BUFFER_SCREEN.currentline_screen=(CURRENT_LINE->screen_line + tmp_line_move);
					BUFFER_SCREEN.current_column=tmp_current_column_down;
				}
				else
					BUFFER_SCREEN.currentline_screen=(CURRENT_LINE->screen_line + tmp_line_move);
				//mvprintw(SCREEN_STATUS,0,"Index-%d\tData_int-%d\tChar-%c\tMax Column-%d",BUFFER_SCREEN.current_index,(int)CURRENT_ATOM->data[BUFFER_SCREEN.current_index],CURRENT_ATOM->data[BUFFER_SCREEN.current_index],max_column);
				move(BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
			}
			break;
		}
		case KEY_LEFT:
		{
			if (1 == BUFFER_SCREEN.current_char_count)	//when cursor is at the first char of the line
				return 1;
			else
			{
				if (2 == BUFFER_SCREEN.current_char_count)//when we are about to move to the left most position of the line
				{
					if (1 == CURRENT_LINE->line_number)
						to_super_critical();
					else
						to_critical();
					BUFFER_SCREEN.current_column=0;
					max_column=0;	//sets max_column always to the column number last moved when in left/right navigation
				}
				else			//when we move one char to the left,may be any number of chars on screen
				{
					char tmp_char_left=CURRENT_ATOM->data[BUFFER_SCREEN.current_index];	//stores the last char moved

					BUFFER_SCREEN.current_char_count--;
					if (0 != BUFFER_SCREEN.current_index)
						BUFFER_SCREEN.current_index--;
					else
					{
						CURRENT_ATOM=CURRENT_ATOM->previous_atom;
						BUFFER_SCREEN.current_index=(MAX_SIZE - 1);
					}
					while (JERK_NOP == CURRENT_ATOM->data[BUFFER_SCREEN.current_index])
					{
						if (0 == BUFFER_SCREEN.current_index)
						{
							CURRENT_ATOM=CURRENT_ATOM->previous_atom;
							BUFFER_SCREEN.current_index=(MAX_SIZE - 1);
						}
						else
							BUFFER_SCREEN.current_index--;
					}
					if (0 == c_x)	//when moving across a wraped line
						BUFFER_SCREEN.currentline_screen--;
					if ('\t' == tmp_char_left)
					{
						BUFFER_SCREEN.current_column=(jerk_virtual_screen(CURRENT_LINE->first_atom,0,CURRENT_ATOM,BUFFER_SCREEN.current_index)%10000);
						if (0 == BUFFER_SCREEN.current_column)		//this takes out the vulnerability of having only one char at the end of the line when being wraped
							BUFFER_SCREEN.current_column=(BUFFER_SCREEN.max_x - 1);
					}
					else
						BUFFER_SCREEN.current_column--;
					max_column=((BUFFER_SCREEN.currentline_screen - CURRENT_LINE->screen_line)*(BUFFER_SCREEN.max_x - 1) + BUFFER_SCREEN.current_column);
				}
				//mvprintw(SCREEN_STATUS,0,"Index-%d\tData_int-%d\tChar-%c\tMax Column-%d",BUFFER_SCREEN.current_index,(int)CURRENT_ATOM->data[BUFFER_SCREEN.current_index],CURRENT_ATOM->data[BUFFER_SCREEN.current_index],max_column);
				move(BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
			}
			break;
		}
		case KEY_RIGHT:
		{
			if (BUFFER_SCREEN.current_char_count == CURRENT_LINE->char_count)
				return 1;
			else
			{
				if (1 == BUFFER_SCREEN.current_char_count)//when we are about to move away from left most position of the line
				{
					from_critical();
					BUFFER_SCREEN.current_column=max_column;
					//BUFFER_SCREEN.currentline_screen to be set in the below else condition or in KEY_UP and KEY_DOWN
				}
				else
				{
					if ((MAX_SIZE - 1) != BUFFER_SCREEN.current_index)	//this if-else condition is for checking if we are pointing at the last char of data buffer
						BUFFER_SCREEN.current_index++;
					else
					{
						CURRENT_ATOM=CURRENT_ATOM->next_atom;
						BUFFER_SCREEN.current_index=0;
					}
					while (JERK_NOP == CURRENT_ATOM->data[BUFFER_SCREEN.current_index])	//this determines the next char to be pointed in the data buffer
					{
						if ((MAX_SIZE - 1) == BUFFER_SCREEN.current_index)
						{
							CURRENT_ATOM=CURRENT_ATOM->next_atom;
							BUFFER_SCREEN.current_index=0;
						}
						else
							BUFFER_SCREEN.current_index++;
					}
					max_column=((c_y - CURRENT_LINE->screen_line)*(BUFFER_SCREEN.max_x - 1) + c_x);	//to set max_column to the current position/max_column according to that line before changing so
				//OPTIMISE OVER HERE IN THE CODE
					if ((BUFFER_SCREEN.max_x - 1) == c_x)	//when the cursor moves across a wraped line
					{
						max_column++;
						BUFFER_SCREEN.current_column=0;
						BUFFER_SCREEN.currentline_screen=(c_y + 1);
					}
					else	//when we may or maynot move across a wraped line
					{
						if ('\t' == CURRENT_ATOM->data[BUFFER_SCREEN.current_index])
						{
							if ((BUFFER_SCREEN.max_x - TABSIZE - 1) <= c_x)	//when we encounter a tab and less than a or equal to tab chars are left to complete the line
							{
								max_column=(max_column + (BUFFER_SCREEN.max_x - 1 - c_x) + 1);
								BUFFER_SCREEN.current_column=0;
								BUFFER_SCREEN.currentline_screen=(c_y + 1);
							}
							else	//when there is still room to be in the same line
							{
								max_column=(max_column + (TABSIZE - (BUFFER_SCREEN.current_column+TABSIZE)%TABSIZE));
								BUFFER_SCREEN.current_column=BUFFER_SCREEN.current_column + (TABSIZE - (BUFFER_SCREEN.current_column+TABSIZE)%TABSIZE);
							}
						}
						else
						{
							max_column++;
							BUFFER_SCREEN.current_column++;
						}
						
					}
				}
				BUFFER_SCREEN.current_char_count++;
				//mvprintw(SCREEN_STATUS,0,"Index-%d\tData_int-%d\tChar-%c\tMax Column-%d",BUFFER_SCREEN.current_index,(int)CURRENT_ATOM->data[BUFFER_SCREEN.current_index],CURRENT_ATOM->data[BUFFER_SCREEN.current_index],max_column);
				move(BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
			}
			break;
		}
	}
	return 0;
}
static int to_super_critical(void)
{
	BUFFER_SCREEN.current_index=-1;
	CURRENT_ATOM=NULL;
	BUFFER_SCREEN.current_char_count=1;
	return 0;
}
static int to_critical(void)
{
	int index_tmp=0;
	struct line_buffer *move_tmp=CURRENT_LINE->previous_line;

	CURRENT_ATOM=move_tmp->first_atom;
	BUFFER_SCREEN.current_char_count=1;
	while (NULL != CURRENT_ATOM->next_atom)		//in edit functions we need to code accordingly that no shit is left in the last atom(if there is some shit then we can simply orphan it)
		CURRENT_ATOM=CURRENT_ATOM->next_atom;
	while ('\n' != CURRENT_ATOM->data[index_tmp])
		index_tmp++;
	BUFFER_SCREEN.current_index=index_tmp;
	return 0;
}
static int from_critical(void)
{
	int index_tmp=0;

	CURRENT_ATOM=CURRENT_LINE->first_atom;
	while (JERK_NOP == CURRENT_ATOM->data[index_tmp])
	{
		if (15 == index_tmp)
		{
			CURRENT_ATOM=CURRENT_ATOM->next_atom;
			index_tmp=0;
		}
		else
			index_tmp++;
	}
	BUFFER_SCREEN.current_index=index_tmp;
	if ('\t' == CURRENT_ATOM->data[index_tmp])
		max_column=(max_column+TABSIZE);
	else
		max_column++;
	return 0;
}
static int jerk_virtual_screen(struct atomic_buffer *tmp_start_atom,int tmp_start_index,struct atomic_buffer *tmp_atom,int tmp_current_index)//what this function does is-it simulates the behaviour of the screen in just numbers
{
	int virtual_index=tmp_start_index,virtual_column=0,virtual_line=0;
	struct atomic_buffer *virtual_atom=tmp_start_atom;

	while (tmp_atom != virtual_atom)
	{
		for (virtual_index=0;virtual_index<MAX_SIZE;virtual_index++)
		{
			if (JERK_NOP == virtual_atom->data[virtual_index])
				continue;
			if ('\t' == virtual_atom->data[virtual_index])
			{
				if ((BUFFER_SCREEN.max_x - TABSIZE - 1) <= virtual_column)
					virtual_column=0;
				else
					virtual_column += (TABSIZE - (virtual_column + TABSIZE)%TABSIZE);
			}
			else
				virtual_column++;
			if ((BUFFER_SCREEN.max_x - 1) == virtual_column)
			{
				virtual_line++;
				virtual_column=0;
			}
		}
		virtual_atom=virtual_atom->next_atom;
		virtual_index=0;
	}
	for (virtual_index=0;virtual_index<=tmp_current_index;virtual_index++)
	{
		if (JERK_NOP == virtual_atom->data[virtual_index])
			continue;
		if ('\t' == virtual_atom->data[virtual_index])
		{
			if ((BUFFER_SCREEN.max_x - TABSIZE - 1) <= virtual_column)
				virtual_column=0;
			else
				virtual_column += (TABSIZE - (virtual_column + TABSIZE)%TABSIZE);
		}
		else
			virtual_column++;
		if ((BUFFER_SCREEN.max_x - 1) == virtual_column)
		{
			virtual_line++;
			virtual_column=0;
		}
	}
	if ((BUFFER_SCREEN.max_x - 1) == virtual_column)
		return (10000*virtual_line);
	else
		return (10000*virtual_line + virtual_column);
}
static int parse_position(void)
{
	struct atomic_buffer *tmp_parse_atom=NULL;
	int index_tmp=0;
	int parse_max_column=0;
	int tmp_current_column=0;

	if (1 == CURRENT_LINE->char_count)	//when we move to a blank line when moving up or down
	{
		BUFFER_SCREEN.current_column=0;
		if (1 == CURRENT_LINE->line_number)
			to_super_critical();
		else
			to_critical();
	}
	else				//only two ways to exit this else part i.e when we have number of chars on screen more than max_column or when we encounter \n before exceeding the number of chars on screen than max_column
	{
		tmp_parse_atom=CURRENT_LINE->first_atom;
		BUFFER_SCREEN.current_char_count=1;
		while (1)
		{
			while (JERK_NOP == tmp_parse_atom->data[index_tmp])	//finds the next char in the data buffer 
			{
				if ((MAX_SIZE - 1) == index_tmp)
				{
					tmp_parse_atom=tmp_parse_atom->next_atom;
					index_tmp=0;
				}
				else
					index_tmp++;
			}
			BUFFER_SCREEN.current_char_count++;
			if ('\t' == tmp_parse_atom->data[index_tmp])
			{
				if ((BUFFER_SCREEN.max_x - TABSIZE -1) <= tmp_current_column)//care to check if my maths is correct
				{
					parse_max_column=(parse_max_column + (BUFFER_SCREEN.max_x - 1 - tmp_current_column) + 1);//check for bugs as edited while compiling
					tmp_current_column=0;
				}
				else
				{
					parse_max_column = (parse_max_column + (TABSIZE - (tmp_current_column+TABSIZE)%TABSIZE));
					tmp_current_column = (tmp_current_column + (TABSIZE - (tmp_current_column+TABSIZE)%TABSIZE));
				}
			}
			else				//when any char other than tab is encountered
			{
				if ('\n' == tmp_parse_atom->data[index_tmp])
				{
					BUFFER_SCREEN.current_char_count--;
					if (0 == index_tmp)
					{
						tmp_parse_atom=tmp_parse_atom->previous_atom;
						index_tmp=(MAX_SIZE - 1);
					}
					else
						index_tmp--;
					while (JERK_NOP == tmp_parse_atom->data[index_tmp])
					{
						if (0 == index_tmp)
						{
							tmp_parse_atom=tmp_parse_atom->previous_atom;
							index_tmp=(MAX_SIZE - 1);
						}
						else
							index_tmp--;
					}
					CURRENT_ATOM=tmp_parse_atom;
					BUFFER_SCREEN.current_index=index_tmp;
					BUFFER_SCREEN.current_column=tmp_current_column;
					return (parse_max_column/(BUFFER_SCREEN.max_x - 1));
				}
				parse_max_column++;
				if (tmp_current_column == (BUFFER_SCREEN.max_x - 1))
					tmp_current_column=0;
				else
					tmp_current_column++;
			}
			if (parse_max_column >= max_column)
				break;
			if ((MAX_SIZE - 1) == index_tmp)	//this is to check so that we don't have the problem of data overwriting in case where we reach index_tmp at MAX_SIZE
			{
				tmp_parse_atom=tmp_parse_atom->next_atom;
				index_tmp=0;
			}
			else
				index_tmp++;
		}
		CURRENT_ATOM=tmp_parse_atom;
		BUFFER_SCREEN.current_index=index_tmp;
		BUFFER_SCREEN.current_column=tmp_current_column;
	}
	return (parse_max_column/(BUFFER_SCREEN.max_x - 1));
}
int print_mesg(int start_column,int position_opt,const char *msg)//need to increase the support for printing of variables
{
	move(SCREEN_STATUS,0);
	clrtoeol();
	if (NULL != msg)
		mvprintw(SCREEN_STATUS,start_column,msg);
	if (JINV_TRUE == position_opt)			//when position_opt is true then move to the char it points else we stay in the command display line
		move(BUFFER_SCREEN.currentline_screen,BUFFER_SCREEN.current_column);
	refresh();
}

int command_reader(char *command_buffer,int command_string_length)
{
	int loop=0;
	int ch;
	int tmp_y,tmp_x;

	memset(command_buffer,'\0',command_string_length);
	ch=jerk_getch();
	for (loop=0;loop<=(command_string_length - 2);loop++)
	{
		getyx(stdscr,tmp_y,tmp_x);
		if ('\n' == ch)
			break;
		if (KEY_BACKSPACE == ch)
		{
			if (0 == loop)
			{
				move(SCREEN_STATUS,0);
				clrtoeol();
				move(SCREEN_STATUS,0);
				refresh();
				return 0;	//need to remove : from the status bar
			}
			else
			{
				loop--;
				command_buffer[loop]='\0';
				move(tmp_y,(tmp_x - 1));
				delch();
				refresh();
			}
			loop--;
			ch=jerk_getch();
			continue;
		}
		if ((33 == ch) || (ch>64 && ch<91) || (ch>96 && ch<123))
		{
			if ((command_string_length - 1) == loop)
				break;
			command_buffer[loop]=(char)ch;
			addch((const chtype)ch);
			refresh();
			ch=jerk_getch();
			continue;
		}
		else
			loop--;
		ch=jerk_getch();
		continue;
	}
	if ((command_string_length - 1) == loop)//when the command buffer is full i.e we cannot enter more chars
	{
		ch=jerk_getch();
		while ('\n' != ch)
			ch=jerk_getch();
	}
	return 0;
}
int end_screen(int number)//the use of number shall come in case of multiple windows
{
	noraw();
	keypad(stdscr,FALSE);
	endwin();
}
int set_flag_edit(void)
{
	return flag_edit;
}
struct line_buffer *current_first_line(void)
{
	return FIRST_LINE;
}
