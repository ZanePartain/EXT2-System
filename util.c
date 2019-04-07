/*********** util.c file ****************/

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


int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   


int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   


int tokenize(char *pathname)
{
  char *t;

  strcpy(gpath,pathname);

  n = 0; //set path count to 0

  t = strtok(gpath,"/");
  while(t){
      name[n] = t;  //append temp name to pathname
      t = strtok(0,"/");
      n++;
  }
}


// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, disp;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
      //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk  = (ino-1) / 8 + inode_start;
       disp = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + disp;
       // copy INODE to mp->INODE
       mip->INODE = *ip;

       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}


iput(MINODE *mip)
{
 int i, block, offset;
 char buf[BLKSIZE];
 INODE *ip;

 if (mip==0) 
     return;

 mip->refCount--;
 
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write back */
 printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino); 

 block =  ((mip->ino - 1) / 8) + inode_start;
 offset =  (mip->ino - 1) % 8;

 /* first get the block containing this inode */
 get_block(mip->dev, block, buf);

 ip = (INODE *)buf + offset;
 *ip = mip->INODE;

 put_block(mip->dev, block, buf);

} 


//
/*search: searches all INODE Blocks in mip, for name Lab6*/
//
int search(MINODE *mip, char *name)
{
  // YOUR serach() fucntion as in LAB 6
  int i, inode_num, blk, offset;
  char sbuf[BLKSIZE], temp[256], dir_name[256];
  char *char_p;

  strcpy(dir_name,name);
  
  for (i=0; i < 12; i++){         // assume DIR at most 12 direct blocks

      if(mip->INODE.i_block[i] == 0){   //never found the dir name
          return 0; 
      }

      // YOU SHOULD print i_block[i] number here
      get_block(mip->dev, mip->INODE.i_block[i], sbuf);

      dp = (DIR *)sbuf;
      char_p = sbuf;

      while(char_p < sbuf + BLKSIZE){
          strncpy(temp, dp->name, dp->name_len);  //make name a string
          temp[dp->name_len] = 0;                    //ensure null at end
          
          if(strcmp(temp,name) == 0){  //found dir, return inode number
              return dp->inode;
          }

          char_p += dp->rec_len;    //advance char_p by rec_len
          dp = (DIR *)char_p;       //pull dir_p to next entry
      }
  }

  return 0;
}


//
/*getino : path2node lab6*/
//
int getino(char *pathname)
{
  int i, ino, blk, disp;
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;

  if (pathname[0]=='/')
    mip = iget(dev, 2);
  else
    mip = iget(running->cwd->dev, running->cwd->ino);

  tokenize(pathname);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      iput(mip);
      mip = iget(dev, ino);
   }
   return ino;
}
