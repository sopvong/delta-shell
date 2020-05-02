#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "sudba.h"

char *sudba_make_string(char *text) {
  int text_length = strlen(text); 
  char *temp = my_malloc(text_length + 1);
  if (!temp)
    return NULL;

  int j = 0;
  for (int i = 1; /* skip the first " */
       i < text_length - 1; /* skip the last " */
       i++, j++) {
    if (text[i] == '\\') { /* if an escape character */
      i++;
      switch(text[i]) {
      case '\\': 
	temp[j] = '\\';
	break;
      case 'n':
	temp[j] = '\n';
	break;
      case 't':
	temp[j] = '\t';
	break;
      case 'r':
	temp[j] = '\r';
	break;
      case '"':
	temp[j] = '"';
	break;
      default:
	temp[j] = text[i];
	break;
      }
    } else
      temp[j] = text[i];
  }
  temp[j] = '\0'; /* NULL-terminate */
  temp = my_realloc(temp, j + 1); /* Trim as needed */
  return temp;
}

bool sudba_exists(char *table) {
  char schema[strlen(table) + sizeof(DB_SCHEMA_EXT)];
  char data  [strlen(table) + sizeof(DB_DATA_EXT  )];
  sprintf(schema, "%s" DB_SCHEMA_EXT, table);
  sprintf(data  , "%s" DB_DATA_EXT  , table);

  struct stat statbuf;
  /* Do both table files exist and readable and writable? */
  return stat(schema, &statbuf) != -1
      && S_ISREG(statbuf.st_mode) && (statbuf.st_mode & (S_IRUSR | S_IWUSR))
      && stat(data, &statbuf) != -1
      && S_ISREG(statbuf.st_mode) && (statbuf.st_mode & (S_IRUSR | S_IWUSR));
}

static pthread_mutex_t lock_on_locks;
static int active_tables = 0;
static TableLock *locks = NULL;

// Table locking functions
void sudba_lock(char *table) {
  char table_name[strlen(table) + 1];
  table_name[strlen(table)] = '\0';
  sprintf(table_name, "%s", table);
	printf("table name: %s\n", table_name); 
  if (active_tables == 0) { //first table in
	locks = my_realloc(locks, (active_tables+1)*sizeof(TableLock));
	locks[0].table = table_name;
	active_tables = active_tables + 1;
	pthread_mutex_lock(&locks[0].lock);
	}
  else {
	for (int i = 0; i < active_tables; i++) {
			if (!(strcmp(locks[i].table, table))) {
				//pthread_mutex_lock(&locks[i].lock); //QQ:use the locks?
				continue;
			}
			if (i == active_tables-1) { //reached end of for loop and still no match!
				locks = my_realloc(locks, (active_tables+1)*sizeof(TableLock));//QQ: sizeof(TableLock)?
				locks[active_tables].table = table_name;
				active_tables = active_tables + 1;
				pthread_mutex_lock(&locks[i].lock);
				
				
			}
		}
	}
  
  // 1. Check if the table is on the list of locks.
  // 2. If it is not, add another lock.
  // 3. Find the mutex for the table (now, it exists!)
  // 4. Lock it
  // Items 1-3 may be combined
}

void sudba_unlock(char *table) {
  char table_name[strlen(table) + 1];
  table_name[strlen(table)] = '\0';
  sprintf(table_name, "%s", table);
  for (int i = 0; i < active_tables; i++) {
	if (!(strcmp(locks[i].table, table))) {
		memset(locks[i].table, '\0', sizeof(locks[i].table)-1); //for now to delete element
		// locks[i-1].table = locks[i].table
		//realloc(locks, sizeof(locks) - sizeof(locks.table))
		//this is the only way i see to remove a table from the array
		//: is this how we are suppsoed to remove the table? for now we just memset it to null terminte
		pthread_mutex_unlock(&locks[i].lock);
		//we gotta unlock
		}
	if (i == active_tables-1) {//table does not exist?
		//pthread_mutex_unlock(&locks[i].lock); //QQ: what if the table does not exist to unlock?
		}
	}
  // 5. Find the mutex for the table (it exists!)
  // 6. Unlock it
}

// Memory management functions
void *my_malloc(size_t size) {
  void *result = malloc(size);
  if(!result) abort();
  return result;
}

void *my_realloc(void *ptr, size_t size) {
  void *result = realloc(ptr, size);
  if(!result) abort();
  return result;
}

// "I'll be back!"
void sudba_not_implemented(FILE *f)
{
  fprintf(f, HTTP_VER " 501 Not implemented\n\r");
}
