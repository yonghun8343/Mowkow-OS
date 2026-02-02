/**************************************************************************
 *   tip.c                                                                *
 *                                                                        *
 *   Copyright (C) 1999 Chris Allegretta                                  *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 1, or (at your option)  *
 *   any later version.                                                   *
 *                                                                        *
 *   This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *   GNU General Public License for more details.                         *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program; if not, write to the Free Software          *
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                        *
 **************************************************************************/

#include "../include/apilib.h"
#include "nano.h"
#include "compat.h"

char *skipspace(char *p);

filestruct *fileage = 0;
filestruct *cutbuffer = 0;
filestruct *filebot = 0;
filestruct *current = 0;
filestruct *edittop = 0;
filestruct *editbot = 0;

int center_x = 0, center_y = 0;
int current_x = 0, current_y = 0;
int editwinrows = 0;
int editwineob = 0;

int cur_x = 0;
int cur_y = 0;

int placewewant = 0;
int totlines = 0;

void global_init(void);
void do_right(void);
void update_cursor(void);
void update_line(filestruct *f);
// void check_wrap(filestruct *f);
void reset_cursor(void);
void edit_update(filestruct *fileptr);
void center_cursor(void);
void titlebar(void);
void edit_refresh(void);
void do_enter(filestruct *inptr);
void do_backspace(void);

void HariMain(void)
{
    char s[30], *p, *q = 0, *r = 0; // p: 커맨드라인 포인터, q: 파일이름 시작 포인터, r: 파일이름 끝 포인터
    int win_width = COLS * 8 + 16;
    int win_height = LINES * 16 + 36;
    api_initmalloc();
    char *winbuf = (char *)api_malloc(win_width * win_height);
    int win;
    void *edit, *topwin, *bottomwin;

    api_cmdline(s, 30);
    for (p = s; (unsigned char)*p > ' '; p++) { }
    for (; (unsigned char)*p != 0; ) {
		p = skipspace(p);
        if (q != 0) {
			api_putstr("> nano file\n");
			api_end();
		}
		q = p;
		for (; (unsigned char)*p > ' '; p++) { }
		r = p;
    }

    if (r != 0) {
        *r = 0;
    }

    if (q == 0) {
        // 아직 새로운 파일 생성 기능이 구현안됐음
        // q = "newfile.txt";
    }


    win = api_openwin(winbuf, win_width, win_height, -1, "HariNano");
	api_boxfilwin(win, 6, 27, win_width - 6, win_height - 6, 0);

    current_win = win;

    topwin = (void *)1;
    edit   = (void *)2;
    bottomwin = (void *)3;

    // api_fopen(q);

    global_init();

    fileage = api_malloc(sizeof(filestruct));
    fileage->data = api_malloc(2);
    fileage->data[0] = '\n';
    fileage->data[1] = 0;
    fileage->next = 0;
    fileage->prev = 0;

    filebot = fileage;
    current = fileage;

    edittop = fileage;
    editbot = fileage;

    titlebar();

    wmove(edit, 0, 0);
    api_refreshwin(win, 0, 0, win_width, win_height);
    
    for (;;) {
        int key = wgetch(edit);

        if (key == 8) { // Backspace
            do_backspace(); // 작동 안됨
        } else if (key == 10) { // Enter
            do_enter(current); // 작동 안됨
        } else {
            if (key >= 32 && key <= 126) { // 일반 문자
                int len;

                if (current->data == 0) {
                    len = 0;
                    current->data = api_malloc(1);
                    current->data[0] = 0;
                } else {
                    char *p = current->data;
                    len = 0;
                    while(*p++) {
                        len++;
                    }
                }

                current->data = realloc(current->data, len + 2);

                int i;
                for (i=len; i>=current_x; i--) {
                    current->data[i+1] = current->data[i];
                }

                current->data[current_x] = key;
                current_x++;

                update_line(current);
                update_cursor();
            }
        }
        reset_cursor();
    }
    api_end();
}

char *skipspace(char *p)
{
    for (; *p==' '; p++) { }
    return p;
}

void global_init(void)
{
   center_x = COLS / 2;
   center_y = LINES / 2;
   current_x = 0;
   current_y = 0;
   editwinrows = LINES - 5;
   editwineob = editwinrows - 1;
   fileage = 0;
   cutbuffer = 0;
}

void reset_cursor(void)
{
  filestruct *ptr = edittop;

  current_y = 0;

  while (ptr != current && ptr != editbot && ptr->next != 0)
  {
     ptr = ptr->next;
     current_y++;
  }
  wmove(edit, current_y, current_x);
}

void update_line(filestruct *fileptr)
{
   filestruct *filetmp;
   int line = 0;

  for (filetmp = edittop; filetmp != fileptr && filetmp != editbot; filetmp = filetmp->next) line++;

  mvwaddstr(edit, line, 0, filetmp->data);
  wrefresh(edit);

}

void update_cursor(void)
{
   int i = 0;

   wmove(edit, current_y, current_x);

   current = edittop;
   while (i <= current_y - 1 && current->next != 0)
   {
      current = current->next;
      i++;
   }

   wrefresh(edit);

}

void center_cursor(void)
{
   current_y = editwinrows / 2;
   wmove(edit, current_y, current_x);
   wrefresh(edit);
}

void page_down(void)
{
  if (editbot->next != 0 && editbot->next != filebot)
  {
     edit_update(editbot->next);
     center_cursor();
  }
  else if (editbot != filebot)
  {
     edit_update(editbot);
     center_cursor();
  }
  else
     while (current != filebot)
     {
        current = current->next;
        current_y++;
     }

  update_cursor();
}

void do_down(void)
{
   if (current->next != 0)
   {
      if (placewewant > 0)
         current_x = placewewant;

      if (current_x > strlen(current->next->data) - 1)
         current_x = strlen(current->next->data) - 1;
   }
   
   if (current_y < editwineob && current != editbot)
      current_y++;
   else
      page_down();

  wrefresh(edit);
}

void do_right(void)
{
   if (current_x < strlen(current->data) - 1)
      current_x++;
   else
   {
      current_x = 0;
      placewewant = 0;
      do_down();
   }

   placewewant = current_x;
}

void edit_update(filestruct *fileptr)
{
   int lines = 0, i = 0, j = 0;
   filestruct *temp;

   temp = fileptr;
   while (i <= editwinrows / 2 && temp->prev != 0)
   {
      i++;
      temp = temp->prev;
   }
   edittop = temp;

   while (lines <= editwinrows - 1 && lines <= totlines && temp != 0 && temp != filebot)
   {
      mvwaddstr(edit, lines, 0, temp->data);
      temp = temp->next;
      lines++;
   } 

   if (temp == filebot)
   {
      mvwaddstr(edit, lines, 0, filebot->data);
      lines++;
      for (i = lines; i <= editwinrows - 1; i++)
         for(j = 0; j <= COLS - 1; j++)
            mvwaddch(edit, i, j, ' ');

   }
   editbot = temp;

}

void titlebar(void)
{
   wattron(topwin, A_REVERSE);

   int i;
   wmove(topwin, 0, 0);
   for (i=0; i<COLS; i++) {
        waddch(topwin, ' ');
   }

   mvwaddstr(topwin, 0, 4, "HariNano");
   mvwaddstr(topwin, 0, COLS/2 - 4, "New Buffer");

   wattroff(topwin, A_REVERSE);
   wrefresh(topwin);
   reset_cursor();
}

void edit_refresh(void)
{
    int lines = 0, i = 0, j = 0;
    filestruct *temp;

    temp = edittop;
    
    while (lines <= editwinrows - 1 && temp != 0) {
      mvwaddstr(edit, lines, 0, temp->data);
      
      // 줄 내용이 짧으면 나머지 공간을 공백으로 지움
      int len = strlen(temp->data);
      for (j = len; j < COLS; j++) {
          mvwaddch(edit, lines, j, ' ');
      }
      
      temp = temp->next;
      lines++;
    } 

   for (; lines <= editwinrows - 1; lines++) {
       for(j = 0; j < COLS; j++)
            mvwaddch(edit, lines, j, ' ');
   }
   
   editbot = temp;
}

filestruct *make_new_node(filestruct *prevnode)
{
   filestruct *newnode;

   newnode = api_malloc(sizeof(filestruct));
   newnode->data = 0;
   newnode->prev = prevnode;
   newnode->next = 0;

   return newnode;
}

void do_enter(filestruct *inptr)
{
   filestruct *newnode;
   char *tmp;
   
   newnode = make_new_node(inptr);

   tmp = &current->data[current_x];
   
   newnode->data = api_malloc(strlen(tmp) + 2); 
   strcpy(newnode->data, tmp);

   *tmp = 0; 

   newnode->next = inptr->next;
   inptr->next = newnode;
   
   if (newnode->next != 0)
      newnode->next->prev = newnode;

   current = newnode;
   current_x = 0;
   totlines++;

   edit_refresh(); 
   wrefresh(edit);
}

void delete_node(filestruct *fileptr)
{
    if (fileptr->data) {
        api_free(fileptr->data, 0);
    }
    api_free(fileptr, 0);
}

void do_backspace(void)
{
    filestruct *previous;

   if (current_x != 0) {

      memmove(&current->data[current_x - 1], &current->data[current_x], strlen(current->data) - current_x + 1);

      current_x--;
      
      update_line(current); 
   } else {
        if (current == fileage) return; 

        previous = current->prev;
        current_x = strlen(previous->data);

        previous->data = realloc(previous->data, strlen(previous->data) + strlen(current->data) + 1);

        strcat(previous->data, current->data);

        previous->next = current->next;
        if (current->next != 0) current->next->prev = previous;
      
        delete_node(current);

        current = previous;
        totlines--;
      
        edit_refresh(); 
    }
   
   wrefresh(edit);
}
