#include"jinv.h"

struct atomic_buffer *give_atom(int jump,void *ptr,void *CURRENT_ATOM)//this function allocates a memory of the size of atom from our own memory block
{
	struct atomic_buffer *tmp;
/*when jump corresponds to LINE then it is being invoked after creation of a line_buffer chunk else it is being called after the last allocation was an atom,tmp after assignment corresponds to the pointer to the atom
*/
	if (LINE == jump)
		tmp=(ptr + LINE_SIZE);
	else
		tmp=(ptr + ATOM_SIZE);
/*using memset(declared in stdlib.h) we set all the chars to NULL which will be helpful in checking for the end of that line
*/
	memset(tmp->data,'\0',MAX_SIZE);
/*CURRENT_ATOM is set to NULL(at declaration as well as in give_atom()) before this function is called after allocation of a line_buffer struct as the first atom won't have any previous atom to point.
*/
	tmp->previous_atom=CURRENT_ATOM;
	tmp->next_atom=NULL;
	return tmp;
}
struct line_buffer *give_line(void *ptr,struct line_buffer *CURRENT_LINE)//this function allocates a memory of the size of line_buffer from our own memory block
{
	struct line_buffer *tmp;
/*tmp is set to the value of the pointer that points to the newly allocated block of line_buffer,the last allocated line's next_line pointer is set to tmp,line number is set and so as rest of the members of the structures appropriately.
*/
	tmp=(ptr + ATOM_SIZE);
	//CURRENT_LINE->next_line=tmp;
	tmp->line_number=(CURRENT_LINE->line_number + 1);
	tmp->screen_line=-1;
	tmp->previous_line=CURRENT_LINE;
	tmp->next_line=NULL;
/*the value of CURRENT_ATOM is set to NULL as the allocation of new atom assigns the value of the new atom's previous pointer to the CURRENT_ATOM,there is a call to give_atom to allocate memory for new atom*NOTE-THIS IS THE ONLY PLACE WHERE give_atom IS CALLED WITH LINE AS AN ARGUMENT*.
CURRENT_ATOM is set to tmp and tmp returned.
*/
	tmp->first_atom=give_atom(LINE,tmp,NULL);
	tmp->char_count=0;
	return tmp;
}
