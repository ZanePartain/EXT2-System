/************* link.c file **************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char   gpath[256];
extern char   *name[64];
extern int    n;
extern int    fd, dev;
extern int    nblocks, ninodes, bmap, imap, inode_start;
extern char   line[256], cmd[32], pathname[256], sourcepath[256];

#define OWNER  000700
#define GROUP  000070
#define OTHER  000007

#include "string.h"

/**
 * Hard link from the new_file (sourcepath) to old_file (pathname)
 */
int mylink(){
    int old_ino, pino;
    char new_dirname[256]={""},new_basename[120]={""};  //new file dirname & basename
    MINODE *old_mip, *pmip;

    printf("pathname = %s\n",pathname);
    printf("sourcepath = %s\n",sourcepath);

    //get old file inode#
    old_ino = getino(pathname);    

    if(old_ino != 0)  //(if exists)
    {
        //get old file MINODE
        old_mip = iget(dev, old_ino);  

        if(dir_or_file(old_mip) != 1)  //(if not DIR)
        {
            int new_ino = getino(sourcepath); //get new_ino & tokenize sourepath

            if(new_ino == 0){
                printf("GOOD TO GO ~~ LINK BOTH FILES\n");
                dirname_basename(new_dirname,new_basename);  //get dirname & basename of sourcepath

                printf("basename = %s\t",new_basename);
                printf("dirname = %s  ; sourcepath\n",new_dirname);
                
                //get the parent DIR MINODE
                if(strlen(new_dirname) == 0){
                    pino = running->cwd->ino;
                }
                else{
                    pino = getino(new_dirname);  
                }
                pmip = iget(dev, pino);  

                if(search(pmip,new_basename) == 0){

                    //create new entry under parent, w/ old inode#
                    enter_name(pmip, old_ino, new_basename); 

                    old_mip->INODE.i_links_count++;  //increment old mip link count
                    old_mip->dirty = 1;              //mark old mip dirty
                    pmip->dirty = 1;
                }

                //write MINODES
                iput(old_mip);  
                iput(pmip);
            }
            else{
                //basename of new file already exists
                printf("_err: %s ; linker file DNE\n", name[n-1]);
            }
        }
        else{
            //basename of old file path is DIR
            printf("_err: %s ; file_to_link is DIR\n", name[n-1]);
        }
    }
    else{
        return;
    }
}


/**
 * Unlink the specified file (if exists).
 * Delete file from parent DIR, and dec INODES link_count
 */
int unlink(){
    int ino, pino;
    char dirname[256] = {""}, basename[256] = {""};
    MINODE *mip, *pmip;

    ino = getino(pathname);  //get inode# of file

    if(ino != 0)  //(file Exists)
    {
        //get MINODE
        mip = iget(dev,ino);
        //(if MINODE is not a DIR)
        if(!S_ISDIR(mip->INODE.i_mode))  
        {
            dirname_basename(dirname,basename); //get dirname & basename of mip

            //Get parent MINODE
            if(strlen(dirname) == 0){
                pino = running->cwd->ino;
            }
            else{
                pino = getino(dirname);
            }
            pmip = iget(dev, pino);

            //remove child file from parent MINODE
            //mark parent as dirty & write back
            rm_child(pmip,basename);
            pmip->dirty = 1; 
            iput(pmip);

            mip->INODE.i_links_count--;

            if(mip->INODE.i_links_count > 0){
                mip->dirty = 1;
            }
            else{
                //deallocate mip
                free_INODE_BLOCK(mip);
            }

            iput(mip);
        }
        else
        {
            //MINODE is a DIR
            printf("_err: %s ; file_to_unlink is DIR\n", name[n-1]);
        }
    }
    else  //(file DNE)
    {
        return;
    }
}

