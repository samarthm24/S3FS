#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifndef SSSFS
#define SSSFS
#define MAX_BLOCKS 128
#define MAX_DIRECT_POINTERS 10


struct dir_ent
{
    char d_name[256];
    ino_t d_ino;
    off_t d_off;
    unsigned short d_recordlen;
    unsigned char d_type;

};



typedef struct dir_ent dir_ent;

typedef struct file{
	       
	char  data[256];       
	long int offset;
    dir_ent children[MAX_DIRECT_POINTERS];    
	         
}file;


typedef struct inode{
    char * path;                    
    char * name;                    
    char * type;                    
    mode_t permissions;		         
    uid_t user_id;		            
    gid_t group_id;		            
    int num_children;               	            
    time_t a_time;                  
    time_t m_time;                  
    time_t c_time;                  
    time_t b_time;                  
    off_t size;
    int blk_no[MAX_DIRECT_POINTERS];                     
    unsigned long int inode_number;     
    
}inode;








int i_bitmap[MAX_BLOCKS];

int d_bitmap[MAX_BLOCKS];

inode inode_blocks[MAX_BLOCKS];
file file_blocks[MAX_BLOCKS];
#endif