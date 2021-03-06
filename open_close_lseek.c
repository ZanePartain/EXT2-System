/************* open_close_lseek.c file **************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char   gpath[256];
extern char   *name[64];
extern int    n;
extern int    fd, dev;
extern int    nblocks, ninodes, bmap, imap, inode_start;
extern char   line[256], cmd[32], pathname[256];

#define OWNER  000700
#define GROUP  000070
#define OTHER  000007

#include "string.h"

/**
 * Convert the mode_string into an integer rep. that will be returned.
 */
int get_mode(char *mode_string){
    int mode;

    /*Convert mode str input to mode int*/
    if(strcmp(mode_string,"R") == 0){
        mode = 0;
    }
    else if(strcmp(mode_string,"W") == 0){
        mode = 1;
    }
    else if(strcmp(mode_string,"RW") == 0){
        mode = 2;
    }
    else if(strcmp(mode_string,"APPEND") == 0){
        mode = 3;
    }
    else{
        //NONE MODE
        mode = -1;
    }

    return mode;
}


/**
 * Truncate the all of the MINODE's Data blocks, by deallocating them.
 * Then set the MINODE size to 0
 */
int truncate(MINODE *mip){
  int i, inode_num, blk, offset;
  char sbuf[BLKSIZE], tbuf[BLKSIZE];
  char *char_p;
  
    for(int i = 0; i < 12; i++){
        if(mip->INODE.i_block[i] == 0){
            continue;
        }
        //deallocate the BLOCK
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }

    //deallocate indirect blocks
    get_block(dev, mip->INODE.i_block[12],sbuf);
    int *idp = (int *)sbuf;

     while(*idp && idp < sbuf + BLKSIZE){
         bdalloc(dev,*idp);
         idp++;
     }

    //deallocate double indirect blocks
    get_block(dev, mip->INODE.i_block[13],sbuf);
    int *didp = (int *)sbuf;

     while(*didp && didp < sbuf + BLKSIZE){
         //get indirect blocks
         memset(tbuf, 0, BLKSIZE);
         get_block(dev,*didp,tbuf);
         idp = (int *)tbuf;

         while(*idp && idp < tbuf + BLKSIZE){
            //deallocate indirect blocks
            bdalloc(dev,*idp);
            idp++;
         }

         bdalloc(dev, *didp); //deallocate double indirect block
         didp++;             //advance to next DIB
     }

    //touch the FILE
     mip->INODE.i_atime = mip->INODE.i_mtime = mip->INODE.i_ctime = time(0L);
     mip->INODE.i_size = 0; 
     mip->dirty = 1;

  return 0;
}


/**
 * Opens a file for read or write using O_RDONLY, O_WRONLY, O_RDWR.
 * These can be bitwise or-ed with O_CREAT, O_APPEND, O_TRUNC, etc.
 * Return a file descriptor.
 */
int open_file(char *file, char *mode_string){
    //mode := 0|1|2|3 for R|W|RW|APPEND
    char  buf[256];
    int mode;
    int ino, index = -1;
    MINODE *mip; 

    mode = get_mode(mode_string); //get mode

    ino = getino(file); //get file inode#

    if(ino == 0){
        //file doesnt exist; create new file; get inode#
        creat_file(file);
        ino = getino(file);
    }

    mip = iget(dev,ino); //get MINODE of file

    if(dir_or_file(mip) == 0){
        //is a REG FILE
        //create new open file table instance
        OFT *oft = (OFT *)malloc(sizeof(OFT)); 

        oft->mode = mode;
        oft->mptr = mip;
        oft->refCount = 1;

        //set oft offset
        if(mode == 3){
            //mode:=APPEND
            oft->offset = mip->INODE.i_size; 
        }
        else if(mode == 1){
            //mode:=WRITE (W)
            truncate(mip);  
            oft->offset = 0;
        }
        else{
            oft->offset = 0;
        }

        //search for first free fd[index] entry
        for(int i = 0; i < NFD; i++){
            if(running->fd[i] == 0){
                index = i;
                break;
            }
        }

        //assign new OFT to running PROC fd[]
        if(index != -1){
            running->fd[index] = oft;
        }
        else{
            //error
            printf("_err: No free file descriptors\n");
        }

    }
    else{
        //error
        printf("_err: %s is not a REG FILE\n",name[n-1]);
    }


    return index; //return the index (-1) if error
}


/**
 * Close an open File Descriptor. Ensure that it is infact open,
 * then close it in the current running PROC.
 */
int close_file(int fd){

    //check for valid fd
    if(fd < 0 && fd >= NFD){
        printf("_err: INVALID fd\n");
        return -1;
    }

    if(running->fd[fd] != 0){
        //points to an OFT; decrement refCount;
        OFT *oftp = running->fd[fd];
        oftp->refCount--;
        printOFT((*oftp));
        if(oftp->refCount == 0){
            iput(oftp->mptr); 
        }

        
    }

    //clear OFT at index fd
    running->fd[fd] = 0; 
    return 0;
}


/**
 * Set the offset of the open File Descriptor to the
 * new offset position. Then return the old original offset.
 */
int my_lseek(int fd, int position){
    int orig_offset = -1;

    //check for valid fd
    if(fd < 0 && fd >= NFD){
        printf("_err: INVALID fd\n");
        return -1;
    }

    //ensure OFT exists
    if(running->fd[fd] != 0){
        OFT *oftp = running->fd[fd];
        orig_offset  = oftp->offset;
        
        //ensure the position is not out of bounds of file
        if(position >= 0 && position < oftp->mptr->INODE.i_size){
            oftp->offset = position;
        }
        else{
            //error
            printf("_err: new offset position is out of bounds\n");
        }
    }

    return orig_offset;
}