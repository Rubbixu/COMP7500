#include <stdint.h>
#include <stdlib.h>
#include "diskSimulator.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "cpmfsys.h"
#include <ctype.h>

//global structure freelist, marking free space on disk
bool FreeList[NUM_BLOCKS];

//function to allocate memory for a DirStructType (see above), and populate it, given a
//pointer to a buffer of memory holding the contents of disk block 0 (e), and an integer index
// which tells which extent from block zero (extent numbers start with 0) to use to make the
// DirStructType value to return.
DirStructType *mkDirStruct(int index,uint8_t *e) {
  //create new extent to store entry information
  Extent newEntry;
  int i = 0;
  for (i=0; i<EXTENT_SIZE; i++) {
    newEntry[i] = *(e + index*EXTENT_SIZE + i);
  }
  //initialize DirStruct
  DirStructType* newDirStruct = (DirStructType*)malloc(sizeof(DirStructType));
  //put corresponding value into DirStruct
  newDirStruct->status = newEntry[0];
  for (i=0; i<8; i++) {
     if ((newDirStruct->name[i] = (char) newEntry[i+1]) == ' '){
       break;//' ' means end of filename
     }
  }
  newDirStruct->name[i] = '\0';
  for (i=0; i<3; i++) {
     if ((newDirStruct->extension[i] = (char) newEntry[i+9]) == ' '){
       break;//' ' means end of extension
     }       //the situation that space in extension is not considered
  }
  newDirStruct->extension[i] = '\0';
  newDirStruct->XL = newEntry[12];
  newDirStruct->BC = newEntry[13];
  newDirStruct->XH = newEntry[14];
  newDirStruct->RC = newEntry[15];
  for (i=0; i<BLOCKS_PER_EXTENT; i++){
    newDirStruct->blocks[i] = newEntry[i+16]; // array of disk sectors used
  }                                           // empty slots are representated by 0
  return newDirStruct;                        // actual disk number is not required
}

// function to write contents of a DirStructType struct back to the specified index of the extent
// in block of memory (disk block 0) pointed to by e
void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e){
  // basically a reverse version of the previous function
  // put corresponding value from DirStruct to memory
  Extent newEntry;
  int i;
  newEntry[0] = d->status;
  for (i=0; i<8; i++) {
    if (d->name[i] == '\0'){
      break;
    }
    newEntry[i+1] = (uint8_t) d->name[i];
  }
  //fill unused slots with 0
  while (i<8) {
    newEntry[i+1] = 0;
    i++;
  }
  for (i=0; i<3; i++) {
    if (d->extension[i] == '\0'){
      break;
    }
    newEntry[i+9] = (uint8_t) d->extension[i];
  }
  //fill unused slots with 0
  while (i<3) {
    newEntry[i+9] = 0;
    i++;
  }
  newEntry[12] = d->XL;
  newEntry[13] = d->BC;
  newEntry[14] = d->XH;
  newEntry[15] = d->RC;
  for (i=0; i<16; i++){
    newEntry[i+16] = d->blocks[i]; // array of disk sectors used
  }
  // put value in extent to memory
  for (i=0; i<EXTENT_SIZE; i++) {
    *(e + ((int) index)*EXTENT_SIZE + i) = newEntry[i];
  }
}

// populate the FreeList global data structure. freeList[i] == true means
// that block i of the disk is free. block zero is never free, since it holds
// the directory. freeList[i] == false means the block is in use.
void makeFreeList(){
  //read disk0 to memory
  uint8_t e[BLOCK_SIZE];
  blockRead(e,0);
  int i,j,index;
  //initialize freelist
  for (i=1; i<NUM_BLOCKS; i++){
    FreeList[i] = true;
  }
  FreeList[0] = false;
  //find usued extent
  for (i=0; i<32; i++){
    if (*(e+i*32) == 0xe5){
      continue;
    }
    else{
      for (j=0; j<16; j++){
  //read block information from directory entry and set them as in use
        index = (int) *(e+i*32+16+j);
        FreeList[index] = false;
      }
    }
  }
}
// debugging function, print out the contents of the free list in 16 rows of 16, with each
// row prefixed by the 2-digit hex address of the first block in that row. Denote a used
// block with a *, a free block with a .
void printFreeList(){
  int i,j;
  //title
  printf("FREE BLOCK LIST: (* means in-use)\n");
  for (i=0; i<16; i++){
    //prefix
    printf("%2x:",i*16);
    for (j=0; j<16; j++){
      if (FreeList[i*16+j] == true){
        //free block
        printf(" .");
      }
      else{
        //block in use
        printf(" *");
      }
    }
    printf("\n");
  }
}


// internal function, returns -1 for illegal name or name not found
// otherwise returns extent nunber 0-31
int findExtentWithName(char *name, uint8_t *block0){
  int i;
  DirStructType *d;
  //exit if name is invalid
  if (checkLegalName(name) == false){
    return -1;
  }
  //following are repeatedly used codes throughout this project
  //the purpose is to deal with every possible name
  char *filename = "";
  char *extname = "";//initialize filename and extension
  char temp_name[9];
  strcpy(temp_name,name);
  filename = strtok(temp_name,".");
  //if filename is blank
  if (filename == NULL){
    filename ="";
  }
  else{
    extname = strtok(0,".");
    //if extension is missing(<filename> or <filename.)
    if (extname == NULL){
      extname ="";
    }
  }
  //end of such codes
  for (i=0; i<32; i++){
    if (*(block0+i*32) == 0xe5){
      continue;
    }
    else{
      d = mkDirStruct(i,block0);
      //find matching extent
      if ((!strcmp(d->name,filename))&&(!strcmp(d->extension,extname))){
        return i;
      }

    }
  }
  //find nothing
  return -1;
}

// internal function, returns true for legal name (8.3 format), false for illegal
// (name or extension too long, name blank, or  illegal characters in name or extension)
bool checkLegalName(char *name){
  //repeatedly used codes
  char *filename = "";
  char *extname = "";
  char temp_name[64];
  strcpy(temp_name,name);
  filename = strtok(temp_name,".");
  if (filename == NULL){
    filename ="";
  }
  else{
    extname = strtok(0,".");
    if (extname == NULL){
      extname ="";
    }
  }
  //invalid because of length
  if ((strlen(filename) == 0)||(strlen(filename) > 8)||(strlen(extname) > 3)){
    return false;
  }
  //invalid because of characters
  strcpy(temp_name,filename);
  int i,char_in_check;
  for (i=0;i<strlen(filename);i++){
    char_in_check = (int) temp_name[i];
    if (!isalnum(char_in_check)){
      return false;
    }
  }
  return true;
}

// print the file directory to stdout. Each filename should be printed on its own line,
// with the file size, in base 10, following the name and extension, with one space between
// the extension and the size. If a file does not have an extension it is acceptable to print
// the dot anyway, e.g. "myfile. 234" would indicate a file whose name was myfile, with no
// extension and a size of 234 bytes. This function returns no error codes, since it should
// never fail unless something is seriously wrong with the disk
void cpmDir(){
  int i,j,filesize,blockcount;
  uint8_t e[BLOCK_SIZE];
  blockRead(e,0);
  DirStructType *d;
  printf("DIRECTORY LISTING\n");
  for (i=0; i<32; i++){
    if (*(e+i*32) == 0xe5){
      continue;
    }
    else{
      DirStructType *d = mkDirStruct(i,e);
      blockcount = -1;//assume last block is never full, start from -1
      //count every none-0 block, last one
      for(j=0; j<16; j++){
        if (d->blocks[j] != 0){
          blockcount++;
        }
      }
      //if last block is full
      if ((d->RC == 0) && (d->BC == 0)) {
        filesize = 1024*(blockcount+1);
      }
      else{
        filesize = 1024*blockcount + 128*d->RC + d->BC;
      }
      fprintf(stdout,"%s.%s %d\n",d->name,d->extension,filesize);
    }
  }
}

// error codes for next five functions (not all errors apply to all 5 functions)
/*
    0 -- normal completion
   -1 -- source file not found
   -2 -- invalid  filename
   -3 -- dest filename already exists
   -4 -- insufficient disk space
*/

//read directory block,
// modify the extent for file named oldName with newName, and write to the disk
int cpmRename(char *oldName, char * newName){
  int old_ext, check_replicate;
  uint8_t e[BLOCK_SIZE];
  blockRead(e,0);
  //if oldname equals newname
  if (!strcmp(oldName,newName)){
    return 0;
  }
  //if newname is invalid
  if (checkLegalName(newName) == false){
    return -2;
  }
  //repeatedly used code
  char *filename = "";
  char *extname = "";
  char temp_name[9];
  strcpy(temp_name,newName);
  filename = strtok(temp_name,".");
  if (filename == NULL){
    filename ="";
  }
  else{
    extname = strtok(0,".");
    if (extname == NULL){
      extname ="";
    }
  }
  //try to find extent based old name
  old_ext = findExtentWithName(oldName,e);
  if (old_ext == -1){
    return -1;
  }
  //if file with same name as newname already exists
  check_replicate = findExtentWithName(newName,e);
  if (check_replicate != -1){
    return -3;
  }
  DirStructType *d = mkDirStruct(old_ext,e);
  //rename
  strcpy(d->name,filename);
  strcpy(d->extension,extname);
  //write back to disk
  writeDirStruct(d, (uint8_t) old_ext, e);
  blockWrite(e,0);
  return 0;
}

// delete the file named name, and free its disk blocks in the free list
int  cpmDelete(char * name){
  int i,ext_index,block_index;
  uint8_t e[BLOCK_SIZE];
  blockRead(e,0);
  //if name is invalid
  if (checkLegalName(name) == false) {
    return -2;
  }
  //try to find extent by name
  ext_index = findExtentWithName(name,e);
  if (ext_index == -1) {
    return -1;
  }
  DirStructType *d = mkDirStruct(ext_index,e);
  //set entry to unused
  d->status = 0xe5;
  //free disk blocks in the free list
  for (i=0;i<16;i++){
    block_index = d->blocks[i];
    if (block_index != 0){
      FreeList[block_index] = true;
    }
  }
  //set blocks in extent to 0 seems unnecessary
  //write back to disk
  writeDirStruct(d, (uint8_t) ext_index, e);
  blockWrite(e,0);
  return 0;

}

// following functions need not be implemented for Lab 2

int  cpmCopy(char *oldName, char *newName);


int  cpmOpen( char *fileName, char mode);

// non-zero return indicates filePointer did not point to open file
int cpmClose(int filePointer);

// returns number of bytes read, 0 = error
int cpmRead(int pointer, uint8_t *buffer, int size);

// returns number of bytes written, 0 = error
int cpmWrite(int pointer, uint8_t *buffer, int size);
