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
#include "../include/apihan.h"
#include "../include/mylib.h"
#include "tip.h"
#include "compat.h"
#include "proto.h"
#include "han.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

char *skipspace(char *p)
{
   for (; *p==' '; p++) { }
   return p;
}

void finish(int win)
{
    blank_bottombars();
    wrefresh(bottomwin);
    endwin(win);
    api_end();
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

void delete_node(filestruct *fileptr)
{
   nano_free(fileptr->data);
   nano_free(fileptr);
}

filestruct *copy_filestruct(filestruct *src)
{
   filestruct *dst, *tmp, *head, *prev;

   head = copy_node(src);
   dst = head;			/* Else we barf on copying just one line :-) */
   tmp = src->next;
   prev = head;

   while (tmp != 0)
   {
      dst = copy_node(tmp);
      dst->prev = prev;
      prev->next = dst;

      prev = dst;
      tmp = tmp->next;
   }

   dst->next = 0;
   return head;
}

int free_node (filestruct *src)
{
   if (src == 0)
      return 0;

   if (src->next != 0)
      nano_free(src->data);
   nano_free(src);
   return 1;
}

int free_filestruct(filestruct *src)
{
   filestruct *fileptr = src;

   if (src == 0)
      return 0;

   while (fileptr->next != 0)
   {
      fileptr = fileptr->next;
      free_node(fileptr->prev);
   }
   free_node(fileptr);

   return 1;
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
//   wrefresh(edit);

}

void center_cursor(void)
{
   current_y = editwinrows / 2;
   wmove(edit, current_y, current_x);
   wrefresh(edit);
}

void edit_refresh(void)
{
   int lines = 0, j = 0;
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

int statusq(shortcut s[], int slen, char *def, char *msg, ...)
{
   va_list ap;
   char foo[133];
   int ret;

   bottombars(s, slen); 

   va_start(ap, msg);
   // vsnprintf(foo, 132, msg, ap);
   strncat(foo, ": ", 132);
   va_end(ap);

   wattron(bottomwin, A_REVERSE);
   ret = tipgetstr(foo, def, s, slen, (strlen(foo) + 3));
   wattroff(bottomwin, A_REVERSE);


   switch (ret)
   {

      case TIP_FIRSTLINE_KEY:
         do_first_line();
         break;
      case TIP_LASTLINE_KEY:
         do_last_line();
         break;
   }

   /* Then blank the screen */
   blank_statusbar_refresh();


   return ret;
}

int do_yesno(int all, char *msg, ...)
{
   va_list ap;
   char foo[133];
   int kbinput, ok = -1;

   clear_bottomwin();
   wattron(bottomwin, A_REVERSE);
   blank_statusbar_refresh();
   wattroff(bottomwin, A_REVERSE);

   wmove(bottomwin, 1, 0);
   onekey(" Y", "Yes");
   if (all)
      onekey(" A", "All");
   wmove(bottomwin, 2, 0);
   onekey(" N", "No");
   onekey("^C", "Cancel");
   
   va_start(ap, msg);
   // vsnprintf(foo, 132, msg, ap);
   va_end(ap);
   wattron(bottomwin, A_REVERSE);
   mvwaddstr(bottomwin, 0, 0, foo);
   wattroff(bottomwin, A_REVERSE);
   wrefresh(bottomwin);

   reset_cursor();

   while (ok == -1)
   {
      kbinput = wgetch(edit);

      switch (kbinput)
      {
         case 'Y': case 'y':
            ok = 1;
            break;
         case 'N': case 'n':
            ok = 0;
            break;
         case 'A': case 'a':
            if (all)
               ok = 2;
            break;
         case TIP_CONTROL_C:
            ok = -2;
            break;
      }
   }
   /* Then blank the screen */
   blank_statusbar_refresh();

   if (ok == -2)
      return -1;
   else
      return ok;
}

void statusbar(char *msg, ...)
{
   va_list ap;
   char foo[133];
   int start_x = 0;

   va_start(ap, msg);
   // vsnprintf(foo, 132, msg, ap);
   va_end(ap);

   start_x = center_x - strlen(foo) / 2 - 1;

   /* Blank out line */
   blank_statusbar_refresh();

   wmove(bottomwin, 0, start_x);
   wattron(bottomwin, A_REVERSE);

   waddstr(bottomwin, "[ ");
   waddstr(bottomwin, foo);
   waddstr(bottomwin, " ]");
   wattroff(bottomwin, A_REVERSE);
   wrefresh(bottomwin);

   statblank = 25;
}

void total_refresh(int win)
{
   int i, j;

   redrawin(win);
   bottombars(main_list, MAIN_LIST_LEN);
   titlebar();
   for (i = 0 ; i <= LINES - 1; i++);
      for (j = i; j != COLS; j++)
         mvwaddch(edit, i, j, ' ');
   wrefresh(edit);

   edit_refresh();
   reset_cursor();
   wrefresh(edit);
   wrefresh(topwin);
   wrefresh(bottomwin);
}

void previous_line(void)
{
   if (current_y > 0)
      current_y--;
   else
      edit_refresh();

   reset_cursor();
}

void load_file(void)
{
   current = fileage;
   wmove(edit, current_y, current_x);
   edit_update(fileage);
   wrefresh(edit);
}

// void open_file(char *filename)
// {
//    long size, totsize = 0, linetemp = 0;
//    char input[2]; /* buffer */
//    char buf[2000] = ""; 
//    filestruct *fileptr;

//    titlebar();
//    fileptr = fileage;

//    if (stat(filename, &fileinfo) == -1)	/* We have a new file */
//    {
//       statusbar("New File");
//       fileage = malloc(sizeof(filestruct));
//       fileage->data = malloc(2);
//       strcpy(fileage->data, "\n");
//       fileage->prev = NULL;
//       fileage->next = NULL;
//       filebot = fileage;
//       fileptr = fileage;
//       current = fileage;
//    }
//    else if ((file = open(filename, O_RDONLY)) == -1)
//    {
//       statusbar("%s: %s", strerror(errno), filename);
//    }
//    else			/* File is A-OK */
//    {
//       statusbar("Reading File");

//       /* Read the entire file into file struct */
//       while ((size = read(file, input, 1)))
//       {
//          linetemp = 0;
//          if (input[0] == '\n')
//          {
//             if (fileage == NULL)
//             {
//                fileage = malloc(sizeof(filestruct));
//                fileage->data = malloc(strlen(buf)+2);
//                strcpy(fileage->data, buf);
//                strcat(fileage->data, "\n");
//                fileage->prev = NULL;
//                fileage->next = NULL;
//                filebot = fileage;
//                fileptr = fileage;
//             }
//             else
//             {
//                filebot->next = malloc(sizeof(filestruct));
//                filebot->next->data = malloc(strlen(buf)+2);
//                strcpy(filebot->next->data, buf);
//                strcat(filebot->next->data, "\n");
//                filebot->next->prev = filebot;
//                filebot->next->next = NULL;
//                filebot = filebot->next;
//             }
//             totlines++;
//             strcpy(buf, "\0");
//          }
//          else
//          {
//             strncat(buf, input, 1);
//          }

//          totsize += size;
//       }
//       statusbar("Read %d bytes (%d lines)", totsize, totlines);
//       wmove(edit, 0, 0);

//       load_file();
//       close(file);
//    }
// }

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

void add_to_cutbuffer(filestruct *inptr)
{
   filestruct *tmp;

   tmp = cutbuffer;
   if (cutbuffer == NULL)
   {
      cutbuffer = inptr;
      inptr->next = NULL;
      inptr->prev = NULL;
      return;
   }
   else
   {
      while(tmp->next != NULL)
         tmp = tmp->next;
   }

   tmp->next = inptr;
   inptr->prev = tmp;
   inptr->next = NULL;
   cutbottom = inptr;
}

void do_cut_text(filestruct *fileptr)
{
   filestruct *tmp;
      
   tmp = fileptr->next;

   if (!keep_cutbuffer)
   {
      free_filestruct(cutbuffer);
      cutbuffer = 0;
   }

   if (fileptr == fileage)
   {
      if (fileptr->next != 0)
      {
         fileptr = fileptr->next;
         tmp = fileptr;
         fileage = fileptr;
         add_to_cutbuffer(fileptr->prev);
         fileptr->prev = 0;
         edit_update(fileage);
      }
      else
      {
         add_to_cutbuffer(fileptr);
         fileage = make_new_node(0);
      }
   } 
   else
   {
      (fileptr->prev)->next = fileptr->next;
      if (fileptr->next != 0)
         (fileptr->next)->prev = fileptr->prev;
      add_to_cutbuffer(fileptr);
   }

   if (tmp != NULL)
      current = tmp;
   else /* FIXME - wrong */
      tmp = make_new_node(tmp);
   
   if (fileptr == edittop)
      edittop = current;

   edit_refresh();
   wrefresh(edit);

   // dump_buffer(cutbuffer);
   reset_cursor();

   keep_cutbuffer = 1;
}

void do_uncut_text(filestruct *fileptr)
{
   filestruct *tmp = fileptr, *newbuf, *newend;

   if (cutbuffer == 0 || fileptr == 0)
      return;	/* AIEEEEEEEEEEEE */

   newbuf = copy_filestruct(cutbuffer);
   /* Make newend = last element in newbuf */
   for (newend = newbuf; newend->next != 0 && newend != 0; 
           newend = newend->next)
      ;

   /* Hook newbuf into fileptr */
   if (fileptr != fileage)
   {
      tmp = fileptr->prev;
      tmp->next = newbuf;
      newbuf->prev = tmp;
   }
   else
      fileage = newbuf;

   /* Connect the end of the buffer to the filestruct */
   newend->next = fileptr;
   fileptr->prev = newend;
   edit_update(current);
   reset_cursor();
   wrefresh(edit);

   // dump_buffer(cutbuffer);
   // dump_buffer(fileage);
}

void do_early_abort(void)
{
   blank_statusbar_refresh();
   bottombars(main_list, MAIN_LIST_LEN);
   reset_cursor();
}

int search_init(void)
{
   int i;

   if (strcmp(last_search, ""))	/* There's a previous search stored */
   {
      if (case_sensitive)
         i = statusq(whereis_list, WHEREIS_LIST_LEN, "", 
                     "Case Sensitive Search [%s]", last_search);
      else
         i = statusq(whereis_list, WHEREIS_LIST_LEN, "", "Search [%s]", 
                     last_search);

      if (i == -1) /* Aborted enter */
         strncpy(answer, last_search, 132);
      else if (i == 0) /* They actually entered something */
      {
         strncpy(last_search, answer, 132);

         /* Blow away last_replace because they entered a new search
            string....uh, right? =) */
         strcpy(last_replace, "");
      }
      else if (i == TIP_CASE_KEY) /* They asked for case sensitivity */
      {
         case_sensitive = 1 - case_sensitive;
         return 1;
      }
      else /* First page, last page, for example could get here */
      {
         do_early_abort();
         return -1;
      }
   }
   else /* last_search is empty */
   {
      if (case_sensitive)
         i = statusq(whereis_list, WHEREIS_LIST_LEN, "",
                     "Case Sensititve Search");
      else
         i = statusq(whereis_list, WHEREIS_LIST_LEN, "", "Search");
      if (i == -1)
      {
         statusbar("Aborted");
         reset_cursor();
         return -1;
      }
      else if (i == 0) /* They entered something new */
         strncpy(last_search, answer, 132);
      else if (i == TIP_CASE_KEY) /* They want it case sensitive */
      {
         case_sensitive = 1 - case_sensitive;
         return 1;
      }
      else /* First line key, etc. */
      {
         do_early_abort();
         return -1;
      }
   }

   return 0;
}

filestruct *findnextstr(int quiet, filestruct *begin, char *needle)
{
   filestruct *fileptr;
   char *searchstr, *found, *tmp;

   fileptr = current;

   searchstr = &current->data[current_x+1]; 
   /* Look for searchstr until EOF */
   while (fileptr != 0 && 
         (found = strstrwrapper(searchstr, needle)) == 0)
   {
       fileptr = fileptr->next;

      if (fileptr == begin)
         return 0;

      if (fileptr != 0)
         searchstr = fileptr->data;
   }

   /* If we're not at EOF, we found an instance */
   if (fileptr != 0)
   {
      current = fileptr;
      current_x = 0;
      for (tmp = fileptr->data; tmp != found; tmp++)
         current_x++;

      edit_update(current);
      reset_cursor();
   }
   else	/* We're at EOF, go back to the top, once */
   {
      fileptr = fileage;

      while(fileptr != current && fileptr != begin && 
            (found = strstrwrapper(fileptr->data,  needle)) == 0)
         fileptr = fileptr->next;

      if (fileptr == begin)
         return 0;

      if (fileptr != current)	/* We found something */
      {
         current = fileptr;
         current_x = 0;
         for (tmp = fileptr->data; tmp != found; tmp++)
            current_x++;

         edit_update(current);
         reset_cursor();

         if (!quiet)
            statusbar("Search Wrapped");
      }
      else	/* Nada */
      {
         if (!quiet)
            statusbar("Search string not found");
         return 0;
      }
   }

   return fileptr;
}

void do_search(void)
{
   int i;

   if ((i = search_init()) == -1)
      return;
   else if (i == 1)
   {
      do_search();
      return;
   }

   findnextstr(0, current, answer);
}

void print_replaced(int num)
{
   if (num > 1)
      statusbar("Replaced %d occurences", num);
   else if (num == 1)
      statusbar("Replaced 1 occurence");
}

void do_replace (void)
{
   int i, j, replaceall = 0, numreplaced = 0, beginx;
   filestruct *fileptr, *begin;
   char *tmp, *copy, prevanswer[132] = "";

   if ((i = search_init()) == -1)
      return;
   else if (i == 1)
   {
      do_replace();
      return;
   }

   strncpy(prevanswer, answer, 132);

   if (strcmp(last_replace, ""))	/* There's a previous replace str */
   {
      i = statusq(replace_list, REPLACE_LIST_LEN, "", 
                     "Replace with [%s]", last_replace);

      if (i == -1) /* Aborted enter */
         strncpy(answer, last_replace, 132);
      else if (i == 0) /* They actually entered something */
         strncpy(last_replace, answer, 132);
      else if (i == TIP_CASE_KEY) /* They asked for case sensitivity */
      {
         case_sensitive = 1 - case_sensitive;
         do_replace();
         return;
      }
      else /* First page, last page, for example could get here */
      {
         do_early_abort();
         return;
      }
   }
   else /* last_search is empty */
   {
      i = statusq(replace_list, REPLACE_LIST_LEN, "", "Replace with");
      if (i == -1)
      {
         statusbar("Aborted");
         reset_cursor();
         return;
      }
      else if (i == 0) /* They entered something new */
         strncpy(last_replace, answer, 132);
      else if (i == TIP_CASE_KEY) /* They want it case sensitive */
      {
         case_sensitive = 1 - case_sensitive;
         do_replace();
         return;
      }
      else /* First line key, etc. */
      {
         do_early_abort();
         return;
      }
   }
   
   begin = current;
   beginx = current_x;
   while (1)
   {

      if (replaceall)
         fileptr = findnextstr(1, begin, prevanswer);
      else 
         fileptr = findnextstr(0, begin, prevanswer);

      if (fileptr == NULL)
      {
         current = begin;
         current_x = beginx;
         edit_update(current);
         print_replaced(numreplaced);
         return;
      }
   
      /* If we're here, we've found the search string */
      if (!replaceall)
         i = do_yesno(1, "Replace this instance?");

      if (i == 1 || replaceall) /* Yes, replace it!!!! */
      {

         /* FIXME - lots of ugly code */
         copy = nano_malloc(strlen(current->data) - strlen(last_search) + 
                       strlen(last_replace) + 1);

         strncpy(copy, current->data, current_x);
         copy[current_x] = 0;

         strcat(copy, last_replace);

         for (j = 1, tmp = current->data; j <= 
             (strlen(last_search) + current_x) && *tmp != 0; j++)
            tmp++;

         if (*tmp != 0)
            strcat(copy, tmp);

         tmp = current->data;
         current->data = copy;

         nano_free(tmp);

         edit_refresh();

         if (!modified)
         {
            modified = 1;
            titlebar();
         }
         numreplaced++;
      }
      else if (i == 2) /* replace all, aieeeeeeeeeeeeee */
         replaceall = 1;
      else if (i == -1) /* Abort, else do nothing and continue loop */
         break;
      }

   print_replaced(numreplaced);
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

void do_up(void)
{
   if (current->prev != 0)
   {
      if (placewewant > 0)
         current_x = placewewant;

      if (current_x > strlen(current->prev->data) - 1)
         current_x = strlen(current->prev->data) - 1;
   }

   if (current_y > 0)
      current_y--;
   else
      page_up();

  wrefresh(edit);
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

void do_left(void)
{
   if (current_x > 0)
      current_x--;
   else if (current != fileage)
   {
      current_x = strlen(current->prev->data) - 1;
      placewewant = 0;
      do_up();
   }
   else
      statusbar("Beep!");

   placewewant = current_x;
}

void delete_buffer(filestruct *inptr)
{
   if (inptr != NULL)
   {
      delete_buffer(inptr->next);
      nano_free(inptr->data);
      nano_free(inptr);
   }
}

void do_backspace(void)
{
   filestruct *previous;

   if (current_x != 0)
   {
      /* Let's get dangerous */
      memmove(&current->data[current_x - 1], &current->data[current_x], 
              strlen(current->data) - current_x + 2);
      current->data = realloc(current->data, strlen(current->data) + 1);
      current_x--;
   }
   else
   {
      if (current == fileage)
         return;	/* Can't delete past top of file */

      previous = current->prev;
      current_x = strlen(previous->data) - 1;
      previous->data = realloc(previous->data,
                       strlen(previous->data) + strlen(current->data) + 2);
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

void do_enter(filestruct *inptr)
{
   filestruct *new;
   char *tmp;

   new = make_new_node(inptr);

   tmp = &current->data[current_x];
   new->data = malloc(strlen(tmp) + 2);
   strcpy(new->data, tmp);
   *tmp++ = '\n';
   *tmp = 0;

   new->next = inptr->next;
   inptr->next = new;
   new->next->prev = new;

   current = new;
   current_x = 0;

   inptr->data = realloc(inptr->data, strlen(inptr->data) + 2);  

   if (current_y == editwinrows - 1)
      edit_update(current);
   else
      edit_refresh();

   reset_cursor();
   wrefresh(edit);
   totlines++;

}

// void do_wrap(filestruct *inptr)
// {
//    filestruct *new;
//    char *tmp, *foo;
//    int backup = 0, jumptonext = 0;

//    new = make_new_node(inptr);

//    tmp = inptr->data + COLS - 1;
//    while (tmp != inptr->data && *tmp == ' ')
//    {
//       tmp--;
//       backup++;
//    }
//    while (tmp != inptr->data && *tmp != ' ')
//    {
//       tmp--;
//       backup++;
//    }

//    if (backup > COLS - current_x)
//       jumptonext = 1;

//    if (tmp == inptr->data)
//       return;
//    tmp++;

//    new->data = nano_malloc(strlen(tmp) + 2);

//    strcpy(new->data, tmp);
//    *tmp++ = '\n';
//    *tmp++ = 0;
//    inptr->data = realloc(inptr->data, strlen(inptr->data) + 1);  

//    if (inptr->next != 0 && inptr->next->wrapline == 1)
//    {
//       foo = nano_malloc(strlen(new->data) + strlen(inptr->next->data) + 2);
//       strcpy(foo, new->data);
//       strip_newline(foo);
//       strcat(foo, inptr->next->data);
//       inptr->next->data = foo;

//       nano_free(new);
//    }
//    else
//    {
//       fflush(stderr);

//       new->next = inptr->next;
//       inptr->next = new;
//       new->next->prev = new;
//       new->wrapline = 1;
//    }

//    if (jumptonext == 1)
//    {
//       current = inptr->next;
//       current_x = backup - (COLS - 1 - current_x);
//    }
//    else if (current_x == COLS - 1)
//    {
//       current = inptr->next;
//       current_x = strlen(new->data)-1;
//    }
   
//    edit_refresh();
//    reset_cursor();
//    wrefresh(edit);
//    totlines++;

// }

// void check_wrap(filestruct *inptr)
// {
//    if ((int) strlen(inptr->data) <= COLS)
//       return;
//    else
//       do_wrap(inptr);
// }

void do_gotoline(void)
{
   long line, i = 1, j = 0;
   filestruct *fileptr;

   j = statusq(replace_list, REPLACE_LIST_LEN, "", "Enter line number");
   if (j == -1)
   {
      statusbar("Aborted");
      reset_cursor();
      return;
   }
   else if (j != 0)
   {
      do_early_abort();
      return;
   }

   if (!strcmp(answer, "$"))
   {
      current = filebot;
      current_x = 0;
      edit_update(current);
      reset_cursor();
      return;
   }

   line = my_atoi(answer);
   /* Bounds check */
   if (line <= 0)
   {
      statusbar("Come on, be reasonable");
      return;
   }

   if (line > totlines) /* FIXME - make totlines update when a new line is
			   added / lines are uncut */
   {
      statusbar("Only %d lines available, skipping to last line", totlines);
      current = filebot;
      current_x = 0;
      edit_update(current);
      reset_cursor();
   }
   else
   {
      for (fileptr = fileage; fileptr != NULL && i < line; i++)
         fileptr = fileptr->next;

      current = fileptr;
      current_x = 0;
      edit_update(current);
      reset_cursor();
   }   

}

void wrap_reset(void)
{
   if (current != NULL)
      current->wrapline = 0;
   else
      return;

   if (current->next != NULL)
      current->next->wrapline = 0;
}

// int write_file(char *name)
// {
//    long size, totsize = 0, linetemp = 0, lineswritten = 0;
//    char input[2]; /* buffer */
//    filestruct *fileptr;

//    titlebar();
//    fileptr = fileage;

//    if ((file = open(name, O_WRONLY | O_CREAT | O_TRUNC)) == -1)
//    {
//       statusbar("Could not open file for writing: %s", strerror(errno));
//       return -1;
//    }

//       dump_buffer(fileage);
//       /* Read the entire file into file struct */
//       while (fileptr != NULL)
//       {
//          size = write(file, fileptr->data, strlen(fileptr->data));
//          if (size == -1)
//          {
//             statusbar("Could not open file for writing: %s",
//                       strerror(errno));
//             return -1;
//          }
//          else
//          {
// #ifdef DEBUG
//             fprintf(stderr, "Wrote >%s", fileptr->data);
// #endif
//          }
//          fileptr = fileptr->next;
//          lineswritten++;
//       }

//    statusbar("Wrote %d lines", lineswritten);
//    return 0;
// }

// void do_writeout(void)
// {
//    int i;

//    i = statusq(writefile_list, WRITEFILE_LIST_LEN, filename, 
//                   "File Name to write");
//    if (i != -1)
//    {

// #ifdef DEBUG
//       fprintf(stderr, "filename is %s", answer);     
// #endif

//       i = write_file(answer);
//    }

//    return;
// }

void do_exit(int win)
{
   int i;

   if (!modified)
      finish(win);

   i = do_yesno(0, "Save modified buffer (ANSWERING \"No\" WILL DESTROY CHANGES) ?");

   if (i == 1)
      // do_writeout();

   if (i != -1) 
      finish(win);

}

char *winbuf_global;
int win_width_global;

HANGUL_STATE h_state;
int lang_mode = 0;

void HariMain(void)
{
   char s[30], *p, *q = 0, *r = 0; // p: 커맨드라인 포인터, q: 파일이름 시작 포인터, r: 파일이름 끝 포인터
   int win_width = COLS * 8 + 48;
   int win_height = LINES * 16 + 36;

   api_initmalloc();
   char *winbuf = (char *)nano_malloc(win_width * win_height);

   winbuf_global = winbuf;
   win_width_global = win_width;

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

   if (r != 0) { *r = 0; }

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
   
   hangul_init(&h_state);
   lang_mode = 1; // 한글 모드로 시작

   for (;;) {
      int key = wgetch(edit);

      switch(key) {
         case 0x3B:
            if (lang_mode == 1) commit_state(&h_state);
            lang_mode ^= 1;
            break;
         case 127:
            if (lang_mode == 1 && h_state.state > 0) {
               commit_state(&h_state);
               do_backspace();
            } else {
               do_backspace();
            }
            break;
         case 13:
            if (lang_mode == 1) commit_state(&h_state);
            do_enter(current);
            break;
         case 8:
            if (lang_mode == 1) commit_state(&h_state);
            // wrap_reset();
            do_up();
            update_cursor();
            keep_cutbuffer = 0;
            check_statblank();          
            break;
         case 2:
            if (lang_mode == 1) commit_state(&h_state);
            // wrap_reset();
            do_down();
            update_cursor();
            keep_cutbuffer = 0;
            check_statblank();          
            break;
         case 4:
            if (lang_mode == 1) commit_state(&h_state);
            do_left();
            update_cursor();
            keep_cutbuffer = 0;
            check_statblank();          
            break;
         case 6:
            if (lang_mode == 1) commit_state(&h_state);
            do_right();
            update_cursor();
            keep_cutbuffer = 0;
            check_statblank();          
            break;
	      case TIP_EXIT_KEY:
            do_exit(win);
            break;
         case TIP_WRITEOUT_KEY:
            // do_writeout();
            bottombars(main_list, MAIN_LIST_LEN);
            break;
         case TIP_GOTO_KEY:
            wrap_reset();
            do_gotoline();
            keep_cutbuffer = 0;
            bottombars(main_list, MAIN_LIST_LEN);
            break;
         case TIP_WHEREIS_KEY:
            // wrap_reset();
            do_search();
            keep_cutbuffer = 0;
            bottombars(main_list, MAIN_LIST_LEN);
            wrefresh(bottomwin);
            break;
         case TIP_CUT_KEY:
            do_cut_text(current);
            break;
         case 1:		/* ^A */
            current_x = 0;
            placewewant = 0;
            break;
         case TIP_INSERTFILE_KEY:
            // wrap_reset();
            keep_cutbuffer = 0;
            break;
         case TIP_PREVPAGE_KEY:
            // wrap_reset();
            current_x = 0;
            page_up();
            keep_cutbuffer = 0;
            check_statblank();          
            break;
         case TIP_NEXTPAGE_KEY:
            // wrap_reset();
            current_x = 0;
            page_down();
            keep_cutbuffer = 0;
            check_statblank();          
            break;
         case TIP_UNCUT_KEY:
            // wrap_reset();
            do_uncut_text(current);
            keep_cutbuffer = 0;
            break;
         case TIP_SPELL_KEY:
            keep_cutbuffer = 0;
            break;
         case TIP_REPLACE_KEY:
            do_replace();
            keep_cutbuffer = 0;
            bottombars(main_list, MAIN_LIST_LEN);
            break;
         case TIP_REFRESH_KEY:
            total_refresh(win);
            break;
         default:
            if (lang_mode == 0) {
               insert_char_at_cursor(key);
               update_line(current);
               wrefresh_rows(current_y, current_y);
            } else {
               hangul_process_key(&h_state, key);
            }
            break;
         }
      reset_cursor();
   }
   finish(win);
   api_end();
}

/* 현재 커서 위치에 1바이트 삽입 (UTF-8 시퀀스 구성용) */
void insert_char_at_cursor(int key) {
    int len = (current->data == 0) ? 0 : my_strlen(current->data);
    current->data = realloc(current->data, len + 2);
    
    int i;
    // 데이터 밀기
    for(i = len; i >= current_x; i--) {
        current->data[i+1] = current->data[i];
    }
    
    current->data[current_x] = key;
    current_x++;
}

/* 현재 커서 앞의 1바이트 삭제 (백스페이스) */
void delete_char_at_cursor() {
    if (current_x <= 0) return;
    
    int len = my_strlen(current->data);
    
    // 데이터 당기기
    int i;
    for(i = current_x - 1; i < len; i++) {
        current->data[i] = current->data[i+1];
    }
    
    current->data = realloc(current->data, len); // 크기 줄임
    current_x--;
}
