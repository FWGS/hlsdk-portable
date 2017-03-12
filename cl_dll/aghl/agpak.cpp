//++ BulliT

#include "agglobal.h"
#include "agpak.h"

typedef struct
{ unsigned char magic[4];             // Name of the new WAD format
  long diroffset;                     // Position of WAD directory from start of file
  long dirsize;                       // Number of entries * 0x40 (64 char)
} pakheader_t;


typedef struct
{ 
  unsigned char filename[0x38];       // Name of the file, Unix style, with extension,
                                      // 50 chars, padded with '\0'.
  long offset;                        // Position of the entry in PACK file
  long size;                          // Size of the entry in PACK file
} pakentry_t;


AgPak::AgPak()
{
}

bool AgPak::GetEntries(const AgString& sPakfile, const AgString& sSearch1, const AgString& sSearch2, AgStringList& lstEntries)
{
  AgString sSearchString1;
  AgString sSearchString2;
  sSearchString1 = sSearch1;
  sSearchString2 = sSearch2;
  AgToLower(sSearchString1);
  AgToLower(sSearchString2);

  pakheader_t Header;
  DWORD dwRead = 0;
  HANDLE h = CreateFile(sPakfile.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,0);

  //Read header.
  if (!ReadFile(h,(void*)&Header,sizeof(Header),&dwRead,NULL))
    return false;
  if (sizeof(Header) != dwRead)
    return false;

  //Move to directory list.
  SetFilePointer(h,Header.diroffset,NULL,FILE_BEGIN);

  //Allocate array of entries.
  pakentry_t* pEntries = (pakentry_t*)malloc(Header.dirsize);

  //Read the entries.
  if (!ReadFile(h,(void*)pEntries,Header.dirsize,&dwRead,NULL))
  {
    free(pEntries);
    CloseHandle(h);
    return false;
  }
  CloseHandle(h);

  if (Header.dirsize != (long)dwRead)
  {
    free(pEntries);
    return false;
  }

  //Calc number of entries.
  int iEntries = Header.dirsize / sizeof(pakentry_t);

  //Read directory listing
  for (int i = 0; i < iEntries; i++)
  {
    AgString sFilename;
    sFilename = (const char*)pEntries[i].filename;
    AgToLower(sFilename);

    if (0 != sSearchString1.length())
    {
      //Check if the file contains the search1 string.
      if (NPOS != sFilename.find(sSearchString1))
      {
        if (0 != sSearchString2.length())
        {
          if (NPOS != sFilename.find(sSearchString2))
            lstEntries.push_back(sFilename);
        }
        else
          lstEntries.push_back(sFilename);
      }
    }
    else
    {
      //Add all files
      lstEntries.push_back(sFilename);
    }
  }
 
  free((void*)pEntries);
  return true;
}


/*
Quake PAK Format

Figured out by Pete McCormick (petra@force.net)  I am not responsible for any damage this does, enjoy, 
and please email me any comments!
Pete

=Format=
Header
(4 unsigned chars) signature = 'PACK' 
(4 unsigned chars, int) directory offeset
(4 unsigned chars, int) directory lenght

Directory
(56 unsigned chars, char) file name
(4 unsigned chars, int) file position
(4 unsigned chars, int) file lenght

File at each position (? unsigned chars, char) file data
Description - The signature must be present in all PAKs; it's the way Quake knows its a real PAK file. 
The directory offset is where the directory listing starts, and the lenght is its lenght. 
In the actuall directory listing, the three options, 56 unsigned chars of a name, 
the files position and lenght, are repeating until the running total of the length (increment by 64) is reached. 
If the directory lenght mod 64 is not a even number, you know their is something wrong. 
And directories are just handled by a making the name something like "/foobar/yeahs.txt". Short and simple.  

Tips - Put the directory entry at the end of the file, so if you added a file, 
you'd just rewrite the small directory entry instead of all the data.  

Limits - Unknown file limit. Don't create too many though :) I would think around a 1000, 
prehaps 2000 (in which case, 2048 would be reasonible) 
*/

//-- Martin Webrant
