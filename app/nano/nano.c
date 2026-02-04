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
#include "proto.h"

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

filestruct *copy_node(filestruct *src)
{
   filestruct *dst;

   dst = nano_malloc(sizeof(filestruct));
   dst->data = nano_malloc(my_strlen(src->data)+2);

   dst->next = src->next;
   dst->prev = src->prev;
   my_strncpy(dst->data, src->data, my_strlen(src->data));
   return dst;
}

/* Unlink a node from the rest of the struct */
void unlink_node(filestruct *fileptr)
{
   if (fileptr->prev != 0)
      fileptr->prev->next = fileptr->next;
   if (fileptr->next != 0)
      fileptr->next->prev = fileptr->prev;
}


void reset_cursor(void)
{
   filestruct *ptr = edittop;

   current_y = 0;

   while (ptr != current && ptr != editbot && ptr->next != 0) {
      ptr = ptr->next;
      current_y++;
   }
   wmove(edit, current_y, current_x);
}

void blank_bottombars(void)
{
   int i, j;

   for (j = 1; j <= 2; j++)
      for (i = 0; i <= COLS - 1; i++)
         mvwaddch(bottomwin, j, i, ' ');

  reset_cursor();
}

void blank_statusbar(void)
{
   int i;

   for (i = 0; i <= COLS - 1; i++)
      mvwaddch(bottomwin, 0, i, ' ');

  reset_cursor();
}

void blank_statusbar_refresh(void)
{
  blank_statusbar();
  wrefresh(bottomwin);
}

void check_statblank(void)
{
   if (statblank > 1)
      statblank--;
   else if (statblank == 1)
   {
      statblank--;
      blank_statusbar_refresh();
   }
}

int tipgetstr(char *buf, char *def, shortcut s[], int slen, int start_x)
{
   int kbinput = 0, j = 0;
   char inputbuf[132] = "";
   int len = 0;

   blank_statusbar();
   mvwaddstr(bottomwin, 0, 0, buf);

   if (def != 0 && *def != 0) {
      my_strcpy(answer, def);
      my_strcpy(inputbuf, def);
      waddstr(bottomwin, def);
   }
   wrefresh(bottomwin);


   while ((kbinput = wgetch(bottomwin)) != 13) {
      for (j = 0; j <= slen - 1; j++) {
         if (kbinput == s[j].val) {
            my_strcpy(answer, "");
            return s[j].val;
         }
      }

      len = my_strlen(inputbuf);

      if (kbinput >= 32 && kbinput <= 126) {
         if (len < 130) {
            inputbuf[len] = (char)kbinput;
            inputbuf[len + 1] = 0;

            mvwaddstr(bottomwin, 0, 0, buf);
            waddstr(bottomwin, inputbuf);
            wrefresh(bottomwin);
         }
      } else if (kbinput == 127) {
         if (len > 0) {
            inputbuf[len - 1] = 0;

            blank_statusbar();
            mvwaddstr(bottomwin, 0, 0, buf);
            waddstr(bottomwin, inputbuf);
            wrefresh(bottomwin);
         }
      }
   }

    my_strcpy(answer, inputbuf);

    if (answer[0] == 0)
       return -1;
    else
       return 0;
}

void horizbar(void *win, int y)
{
   int i = 0;

   wattron(win, A_REVERSE);
   for (i = 0; i <= COLS - 1; i++)
      mvwaddch(win, y, i, ' ');
   wattroff(win, A_REVERSE);
}

void titlebar(void)
{
   wattron(topwin, A_REVERSE);

   int i;
   wmove(topwin, 0, 0);
   for (i=0; i<COLS; i++) {
        waddch(topwin, ' ');
   }

   mvwaddstr(topwin, 0, 4, "GNU nano");
   mvwaddstr(topwin, 0, COLS/2, "New Buffer");

   wattroff(topwin, A_REVERSE);
   wrefresh(topwin);
   reset_cursor();
}

void onekey(char *keystroke, char *desc)
{
   char description[80];
   int i;

   // snprintf(description, 12, " %-11s", desc);
   description[0] = ' ';
   for (i=0; i<10; i++) {
      if (desc[i] == 0) break;
      description[i+1] = desc[i];
   }
   for(; i<11; i++) description[i+1] = ' ';
   description[i+1] = 0;

   wattron(bottomwin, A_REVERSE);
   waddstr(bottomwin, keystroke);
   wattroff(bottomwin, A_REVERSE);
   waddstr(bottomwin, description);
}

void clear_bottomwin(void)
{
   int i;

   for (i = 0; i <= COLS - 1; i++)
   {
      mvwaddch(bottomwin, 1, i, ' ');
      mvwaddch(bottomwin, 2, i, ' ');
   }
   wrefresh(bottomwin);
}

void bottombars(shortcut s[], int slen)
{
   int i;
   char keystr[10];

   clear_bottomwin();
   wmove(bottomwin, 1, 0);
   for (i = 0; i <= slen - 1; i += 2)
   {
      keystr[0] = '^';
      keystr[1] = s[i].val + 64;
      keystr[2] = 0;

      onekey(keystr, s[i].desc);
   }
   wmove(bottomwin, 2, 0);
   for (i = 1; i <= slen - 1; i += 2)
   {
      keystr[0] = '^';
      keystr[1] = s[i].val + 64;
      keystr[2] = 0;

      onekey(keystr, s[i].desc);
   }
   wrefresh(bottomwin);

}

void update_line(filestruct *fileptr)
{
   filestruct *filetmp;
   int line = 0;

  for (filetmp = edittop; filetmp != fileptr && filetmp != editbot; 
       filetmp = filetmp->next)
     line++;

  mvwaddstr(edit, line, 0, filetmp->data);
  wrefresh(edit);

}

void center_cursor(void)
{
   current_y = editwinrows / 2;
   wmove(edit, current_y, current_x);
   wrefresh(edit);
}

void edit_refresh(void)
{
   int lines = 0, i = 0, j = 0;
   filestruct *temp;

   temp = edittop;
    
   while (lines <= editwinrows - 1 && temp != 0) {
      mvwaddstr(edit, lines, 0, temp->data);
      
      // 줄 내용이 짧으면 나머지 공간을 공백으로 지움
      int len = my_strlen(temp->data);
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

void edit_update(filestruct *fileptr)
{
   int lines = 0, i = 0, j = 0;
   filestruct *temp;

   temp = fileptr;
   while (i <= editwinrows / 2 && temp->prev != 0) {
      i++;
      temp = temp->prev;
   }
   edittop = temp;

   while (lines <= editwinrows - 1 && lines <= totlines && temp != 0 && temp != filebot) {
      mvwaddstr(edit, lines, 0, temp->data);
      temp = temp->next;
      lines++;
   } 

   if (temp == filebot) {
      mvwaddstr(edit, lines, 0, filebot->data);
      lines++;
      for (i = lines; i <= editwinrows - 1; i++)
         for(j = 0; j <= COLS - 1; j++)
            mvwaddch(edit, i, j, ' ');

   }
   editbot = temp;

}

void do_first_line(void)
{
   current = fileage;
   placewewant = 0;
   current_x = 0;
   edit_update(current);
}

void do_last_line(void)
{
   current = filebot;
   placewewant = 0;
   current_x = 0;
   edit_update(current);
}


void previous_line(void)
{
   if (current_y > 0)
      current_y--;
   else
      edit_refresh();

   reset_cursor();
}

void update_cursor(void)
{
   int i = 0;

   wmove(edit, current_y, current_x);

   current = edittop;
   while (i <= current_y - 1 && current->next != 0) {
      current = current->next;
      i++;
   }

   wrefresh(edit);

}

void page_down(void)
{
  if (editbot->next != 0 && editbot->next != filebot) {
      edit_update(editbot->next);
      center_cursor();
   } else if (editbot != filebot) {
      edit_update(editbot);
      center_cursor();
   } else while (current != filebot) {
      current = current->next;
      current_y++;
   }

   update_cursor();
}



filestruct *make_new_node(filestruct *prevnode)
{
   filestruct *newnode;

   newnode = nano_malloc(sizeof(filestruct));
   newnode->data = 0;
   newnode->prev = prevnode;
   newnode->next = 0;

   return newnode;
}

void do_down(void)
{
   if (current->next != 0) {
      if (placewewant > 0)
         current_x = placewewant;

      if (current_x > my_strlen(current->next->data) - 1)
         current_x = my_strlen(current->next->data) - 1;
   }
   
   if (current_y < editwineob && current != editbot)
      current_y++;
   else
      page_down();

   wrefresh(edit);
}

void page_up(void)
{
   if (edittop != fileage)
   {
      edit_update(edittop);
      center_cursor();
   }
   else
      current_y = 0;

   update_cursor();
}

void do_right(void)
{
   if (current_x < my_strlen(current->data) - 1)
      current_x++;
   else {
      current_x = 0;
      placewewant = 0;
      do_down();
   }

   placewewant = current_x;
}

void do_enter(filestruct *inptr)
{
   filestruct *new;
   char *tmp;

   new = make_new_node(inptr);

   tmp = &current->data[current_x];
   new->data = nano_malloc(my_strlen(tmp) + 2);
   strcpy(new->data, tmp);
   *tmp++ = '\n';
   *tmp = 0;

   new->next = inptr->next;
   inptr->next = new;
   new->next->prev = new;

   current = new;
   current_x = 0;

   inptr->data = realloc(inptr->data, my_strlen(inptr->data) + 2);  

   if (current_y == editwinrows - 1)
      edit_update(current);
   else
      edit_refresh();

   reset_cursor();
   wrefresh(edit);
   totlines++;

}

void delete_node(filestruct *fileptr)
{
   if (fileptr->data) {
      nano_free(fileptr->data);
   }
   nano_free(fileptr);
}

void do_backspace(void)
{
   filestruct *previous;

   if (current_x != 0)
   {
      /* Let's get dangerous */
      memmove(&current->data[current_x - 1], &current->data[current_x], 
              my_strlen(current->data) - current_x + 2);
      current->data = realloc(current->data, my_strlen(current->data) + 1);
      current_x--;
   }
   else
   {
      if (current == fileage)
         return;	/* Can't delete past top of file */

      previous = current->prev;
      current_x = my_strlen(previous->data) - 1;
      previous->data = realloc(previous->data,
                       my_strlen(previous->data) + my_strlen(current->data) + 2);
      strip_newline(previous->data);
      strcat(previous->data, current->data);

      unlink_node(current);
      delete_node(current);
      if (current == edittop)
         page_up();
      current = previous;
      previous_line();
      wrefresh(edit);
   }
   if (!modified)
   {
      modified = 1;
      titlebar();
   }

   edit_refresh();
   update_cursor();
   wrefresh(edit);
   keep_cutbuffer = 0;
}

void HariMain(void)
{
   char s[30], *p, *q = 0, *r = 0; // p: 커맨드라인 포인터, q: 파일이름 시작 포인터, r: 파일이름 끝 포인터
   int win_width = COLS * 8 + 48;
   int win_height = LINES * 16 + 36;
   api_initmalloc();
   char *winbuf = (char *)nano_malloc(win_width * win_height);
   int win;

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


   win = api_openwin(winbuf, win_width, win_height, -1, "nano");
	api_boxfilwin(win, 6, 27, win_width - 6, win_height - 6, 0);

   current_win = win;

   topwin = (void *)1;
   edit   = (void *)2;
   bottomwin = (void *)3;

   // api_fopen(q);

   global_init();

   fileage = nano_malloc(sizeof(filestruct));
   fileage->data = nano_malloc(1);
   fileage->data[0] = 0;
   fileage->next = 0;
   fileage->prev = 0;

   filebot = fileage;
   current = fileage;

   edittop = fileage;
   editbot = fileage;

   titlebar();
   bottombars(main_list, MAIN_LIST_LEN);

   wmove(edit, 0, 0);
   api_refreshwin(win, 0, 0, win_width, win_height);
    
   for (;;) {
      int key = wgetch(edit);

      if (key < 32 && key != 13) {
         if (key == TIP_EXIT_KEY) {
            break;
         }
      }
      if (key == 127) { // Backspace
         do_backspace();
      } else if (key == 13) { // Enter
         do_enter(current);
      } else {
         if (key >= 32 && key <= 126) { // 일반 문자
            int len;
               if (current->data == 0) {
                  len = 0;
                  current->data = nano_malloc(1);
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
