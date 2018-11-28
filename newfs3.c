#include "fs.h"
#include <stdio.h>
#include <fuse.h>

#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>





char *file_name = "all";
char *curr_working_directory="/";
int curr_working_directory_ino=0;

void read_from_disk(){
	int i;
	FILE *fp=fopen(file_name,"rb");
	//fread(t2,sizeof(tree),1,fp);
	for(i=0;i<MAX_BLOCKS;i++){
		fread(&i_bitmap[i],sizeof(int),1,fp);
	}
	for(i=0;i<MAX_BLOCKS;i++){
		fread(&d_bitmap[i],sizeof(int),1,fp);
	}
	for(i=0;i<MAX_BLOCKS;i++){
		fread(&inode_blocks[i],sizeof(inode),1,fp);
	}
	for(i=0;i<MAX_BLOCKS;i++){
		fread(&file_blocks[i],sizeof(file),1,fp);
	}
	fclose(fp);
}

void write_into_disk(){
	int i;
	FILE *fp = fopen(file_name,"wb");
	for(i=0;i<MAX_BLOCKS;i++){
		fwrite(&i_bitmap[i],sizeof(int),1,fp);
	}
	for(i=0;i<MAX_BLOCKS;i++){
		fwrite(&d_bitmap[i],sizeof(int),1,fp);
	}
	for(i=0;i<MAX_BLOCKS;i++){
		fwrite(&inode_blocks[i],sizeof(inode),1,fp);
	}
	for(i=0;i<MAX_BLOCKS;i++){
		fwrite(&file_blocks[i],sizeof(file),1,fp);
	}
	fclose(fp);
}

char * extract_name(char *path){
	if(strcmp(path,"/")==0)
		return path;
  	char str[strlen(path)];
  	strcpy(str,path);
    char* token = strtok(str, "/"),*prev_token = malloc(sizeof(char)*strlen(path)); 
    while (token != NULL) { 
    	strcpy(prev_token,token);
        token = strtok(NULL, "/"); 
    } 
    return prev_token;
}

int find_available_inode(){
	int i;
	for(i=0;i<MAX_BLOCKS;i++)
		if(i_bitmap[i]==0){
			i_bitmap[i] = 1;
			return i;
		}
	return -1;
}

int find_available_datanode(){
	int i;
	for(i=0;i<MAX_BLOCKS;i++)
		if(d_bitmap[i]==0){
			d_bitmap[i] = 1;
			return i;
		}
	return -1;
}

int find_file_directory(char *name,int inode_number){
	int no_children = inode_blocks[inode_number].num_children;
	for(int i=0;i<no_children;i++){
		if(strcmp(file_blocks[inode_blocks[inode_number].blk_no[0]].children[i+2].d_name,name)==0)
			return file_blocks[inode_blocks[inode_number].blk_no[0]].children[i+2].d_ino;
	}
	return -1;
}

int find_path(char *path){
	char *buf = "find path called";
	//FILE *fp = fopen(file_name,"a+");
	//fwrite(buf,strlen(buf),1,fp);
	//fclose(fp);
	if(strcmp(path,"/")==0)
		return 0;
  	char str[strlen(path)];
  	strcpy(str,path);
  	int inode_number = 0;
    char* token = strtok(str, "/"),prev_token[128]; //= malloc(sizeof(char)*strlen(path)); 
    while (token != NULL) {
	
    	strcpy(prev_token,token);
	printf("find path prev_token %s, token %s\n",prev_token,token); 
    	inode_number = find_file_directory(prev_token,inode_number);
    	if(inode_number == -1)
    		return -1;
        token = strtok(NULL, "/"); 
    } 
    return inode_number;
}

int make_node(char *path,char* type){
	inode temp;
	temp.inode_number = find_available_inode();
	if(temp.inode_number == -1)
		return -1;
	temp.path = path;
	temp.name = extract_name(path);
	temp.type = type;
	if(strcmp(type,"d")==0)
		temp.permissions = S_IFDIR | 0755;
	else
		temp.permissions = S_IFREG | 0644;
	temp.user_id = getuid();
	temp.group_id = getgid();
	temp.num_children = 0;
	temp.a_time = time(NULL);
	temp.m_time = time(NULL);
	temp.b_time = time(NULL);
	temp.c_time = time(NULL);
	temp.size = 0;
	for(int i=0;i<10;i++)
		temp.blk_no[i] = -1;
	temp.blk_no[0] = find_available_datanode();
	strcpy(file_blocks[temp.blk_no[0]].data,"");
	if(strcmp(type,"d")==0){
		strcpy(file_blocks[temp.blk_no[0]].children[0].d_name,".");
		file_blocks[temp.blk_no[0]].children[0].d_ino=temp.inode_number;
		strcpy(file_blocks[temp.blk_no[0]].children[1].d_name,"..");
		if(strcmp(path,"/")==0){
			file_blocks[temp.blk_no[0]].children[1].d_ino=curr_working_directory_ino;
		}
		else{
			char *temp2=(char *)malloc(sizeof(char)*128);
			if(strlen(path)-strlen(temp.name)-1!=0){
				strncpy(temp2,path,strlen(path)-strlen(temp.name)-1);
				temp2[strlen(path)-strlen(temp.name)-1]='\0';
			}			
			else 
				strcpy(temp2,"/");
			int inode_number = find_path(temp2);
			//printf("path, parent name, child name,path length,parent length, length %s %s %s %d %d %d\n",path,temp2,temp.name,strlen(path),strlen(temp2),strlen(path)-strlen(temp.name)-1);
			//printf("in make node inode number %d\n",inode_number);
			file_blocks[temp.blk_no[0]].children[1].d_ino=inode_number;
		}
	}
	inode_blocks[temp.inode_number]=temp;
	if(strcmp(path,"/")!=0){
		char *temp2=(char *)malloc(sizeof(char)*128);
		if(strlen(path)-strlen(temp.name)-1!=0){
			strncpy(temp2,path,strlen(path)-strlen(temp.name)-1);
			temp2[strlen(path)-strlen(temp.name)-1]='\0';
		}
		else 
			strcpy(temp2,"/");
		int inode_number = find_path(temp2);
		//printf("parent name%d\n",strlen(path)-strlen(temp.name)-1 );
		//printf("in make node inode number %d\n",inode_number);
		inode_blocks[inode_number].num_children++;
		strcpy(file_blocks[inode_blocks[inode_number].blk_no[0]].children[inode_blocks[inode_number].num_children+1].d_name,temp.name);
		file_blocks[inode_blocks[inode_number].blk_no[0]].children[inode_blocks[inode_number].num_children+1].d_ino = temp.inode_number;
	}
	write_into_disk();
	return temp.inode_number;
}

int write_data(char *path,char *data){
	int inode_number = find_path(path);
	//printf("%d\n",inode_number );
	inode_blocks[inode_number].size = strlen(data);
	//printf("hi\n");
	strcpy(file_blocks[inode_blocks[inode_number].blk_no[0]].data,data);
	write_into_disk();
	return strlen(data);
}

char * read_data(char *path){
	int inode_number = find_path(path);
	char *temp = (char *)malloc(sizeof(char)*256);
	strcpy(temp,file_blocks[inode_blocks[inode_number].blk_no[0]].data);
	write_into_disk();
	return temp;
}

int delete_from_parent_directory(char *path,int ino,int parent_ino){
	int i,j;
	for(i=0;i<inode_blocks[parent_ino].num_children+2;i++){
		if(file_blocks[inode_blocks[parent_ino].blk_no[0]].children[i].d_ino == ino){
			j=i;
			break;
		}
	}
	printf("j value%d\n",j );
	//printf("name identified %s inode number %d\n",file_blocks[inode_blocks[parent_ino].blk_no[0]].children[j].d_name,file_blocks[inode_blocks[parent_ino].blk_no[0]].children[j].d_ino );
	for(i=j;i<inode_blocks[parent_ino].num_children+2-1;i++){
		strcpy(file_blocks[inode_blocks[parent_ino].blk_no[0]].children[i].d_name,file_blocks[inode_blocks[parent_ino].blk_no[0]].children[i+1].d_name);
		file_blocks[inode_blocks[parent_ino].blk_no[0]].children[i].d_ino=file_blocks[inode_blocks[parent_ino].blk_no[0]].children[i+1].d_ino;
	}
	inode_blocks[parent_ino].num_children--;
	return 0;
}

int delete_directory_node(const char *path)
{

	if(strcmp(path,"/")==0)
		return -1;
	int parent_ino;
	int ino = find_path(path);
	printf("inode called for delete %d\n",ino );
	if(inode_blocks[ino].num_children != 0)
	{
		return -1;
	}
	//printf("%s\n", file_blocks[inode_blocks[ino].blk_no[0]].children[1].d_name);
	
	if(strcmp(file_blocks[inode_blocks[ino].blk_no[0]].children[1].d_name,"..") == 0)
	{
		parent_ino = file_blocks[inode_blocks[ino].blk_no[0]].children[1].d_ino;
	
	}
	//printf("yo\n");
	printf("parent inode %d\n",parent_ino );
	//printf("%d\n", inode_blocks[parent_ino].num_children);
	delete_from_parent_directory(path,ino,parent_ino);
	//inode_blocks[parent_ino].num_children--;
	//printf("%d\n", inode_blocks[parent_ino].num_children);
	
	i_bitmap[ino] = 0;
	d_bitmap[inode_blocks[ino].blk_no[0]] = 0;
	return 0;	
}

int delete_regular_node(char *path){
	if(strcmp(path,"/")==0)
		return -1;
	int parent_ino;
	int ino = find_path(path);
	printf("inode called for delete%d\n",ino );
	char *temp2=(char *)malloc(sizeof(char)*128);
	char *name=(char *)malloc(sizeof(char)*128);
	strcpy(name,extract_name(path));
	if(strlen(path)-strlen(name)-1!=0)
		strncpy(temp2,path,strlen(path)-strlen(name)-1);
	else 
		strcpy(temp2,"/");
	parent_ino = find_path(temp2);
	delete_from_parent_directory(path,ino,parent_ino);
	i_bitmap[ino] = 0;
	d_bitmap[inode_blocks[ino].blk_no[0]] = 0;
	return 0;
}

int do_rmdir(const char *path)
{
	printf("RMDIR CALLED for path %s\n",path);
	int ret = delete_directory_node(path);
	if(ret < 0)
		return -1;
	return 0;
}

int do_unlink(const char *path){
	printf("UNLINK CALLED for path %s\n",path);
	int ret = delete_regular_node(path);
	if(ret < 0)
		return -1;
	return 0;
}


int do_mkdir(const char * path, mode_t x){
	printf("MKDIR CALLED\n");
	make_node(path,"d");
	printf("inode created %d\n",find_path(path));
	write_into_disk();
	return 0;
}

int do_mknod(const char * path, mode_t x, dev_t y){
	printf("MKNOD CALLED with mode %d dmode %d\n",x,S_IFDIR | 0755);
	char *type=(char *)malloc(sizeof(char));
	//if(x == S_IFDIR | 0755)
	//	strcpy(type,"d");
	//else
	strcpy(type,"f");
	make_node(path,type);
	write_into_disk();
	return 0;
}


int find_file(char* name)
{
    char i;
    int ino=find_path(name);
    if(ino!=-1)
    {
    	return inode_blocks[ino].blk_no[0]; 
    }

    return -1;         
}

int do_open(char *name)
{
    int file_index = find_file(name);
    //printf("%d\n",file_index);
    if(file_index < 0) {  
        fprintf(stderr, "fs_open()\t error: file [%s] does not exist.\n",name);
        return -1;
    }

    printf("fs_open()\t called successfully: file [%s] opened.\n", name);
    return 0;
}

static int do_getattr( const char *path, struct stat *st, struct fuse_file_info *fi )
{
	
	printf( "[getattr] Called\n" );
	
	printf( "\tAttributes of %s requested\n", path );
	char * temp = (char*)path;
	int inode_number = find_path(temp);
	if(inode_number == -1)
		return -ENOENT;
	//printf("hi\n");
	st->st_uid = inode_blocks[inode_number].user_id; 
	st->st_gid = inode_blocks[inode_number].group_id; 
	st->st_atime = time( NULL ); 
	st->st_mtime = time( NULL );
	st->st_mode = inode_blocks[inode_number].permissions;
	st->st_nlink = 1;
	st->st_size = inode_blocks[inode_number].size;
	write_into_disk();
	return 0;
}

/*static int do_getattr( const char *path, inode *i )
{
	
	printf( "[getattr] Called\n" );
	
	printf( "\tAttributes of %s requested\n", path );
	char * temp = (char*)path;
	int inode_num = find_path(temp);
	if(inode_num == -1)
		return -ENOENT;
	//printf("hi\n");
	strcpy(i->path,temp);
	i->inode_number = inode_num;
	i->user_id = inode_blocks[inode_num].user_id; 
	i->group_id = inode_blocks[inode_num].group_id; 
	i->a_time = time(NULL);
	i->m_time = time(NULL);
	i->size = inode_blocks[inode_num].size;
	i->num_children = 1;
	write_into_disk();
	return 0;
}*/
static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	printf( "--> Getting The List of Files of %s\n", path );
	
	
	int inode_number = find_path(path);
	for(int i=0;i<inode_blocks[inode_number].num_children+2;i++){
		filler(buffer,file_blocks[inode_blocks[inode_number].blk_no[0]].children[i].d_name,NULL,0);
	}
	write_into_disk();
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	char * temp=read_data(path);
	memcpy(buffer,  temp + offset, size);
	write_into_disk();
	return size;
}

int do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	printf("WRITE CALLED and path is :%s\n",path);
	int inode_number = find_path(path);
	if(inode_number==-1){
		return 0;
	}
	write_data(path,buf);
	write_into_disk();
	return size;
}
int do_chmod(const char *path, mode_t mode)
{
	char *temp=(char*)path;
	int inode_number = find_path(temp);
	if(strcmp(temp,"\0")==0 || inode_number==-1)
		return -ENOENT;
	inode_blocks[inode_number].permissions = mode;
	inode_blocks[inode_number].c_time = time(NULL);
	return 0;
}
int do_chown(const char *path, uid_t owner, gid_t group)
{
	char *temp=(char*)path;
	int inode_number=find_path(temp);
	if(inode_number==-1 || strcmp(path,"\0")==0)
		return -ENOENT;

	inode_blocks[inode_number].user_id = owner;
	inode_blocks[inode_number].group_id = group;
	return 0;

}
static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
    .mknod      = do_mknod,
    .write      = do_write,
    .mkdir      = do_mkdir,
    .open       = do_open,
    .rmdir      = do_rmdir,
    .unlink     = do_unlink,
    .chown      = do_chown,
    .chmod      = do_chmod,
};

int main( int argc, char *argv[] ){
	read_from_disk();
	return fuse_main( argc, argv, &operations);
}
