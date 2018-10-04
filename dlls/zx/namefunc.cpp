//
// (http://planethalflife.com/botman/)
//
// namefunc.cpp
//

#include "extdll.h"
#include "util.h"
#include "enginecallback.h"
#include "cbase.h"

#include "bot.h"


#define MAX_SYMBOLS 1000 /* max exported symbols */

#define DOS_SIGNATURE   0x5A4D       /* MZ */
#define NT_SIGNATURE    0x00004550   /* PE00 */


//extern GIVEFNPTRSTODLL other_GiveFnptrsToDll;
extern HINSTANCE h_Library;

// globals
WORD *p_Ordinals = NULL;
DWORD *p_Functions = NULL;
DWORD *p_Names = NULL;
char *p_FunctionNames[MAX_SYMBOLS];
int num_ordinals;
unsigned long base_offset;


typedef struct {                        // DOS .EXE header
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
  } DOS_HEADER, *P_DOS_HEADER;

typedef struct {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} PE_HEADER, *P_PE_HEADER;

#define SIZEOF_SHORT_NAME              8

typedef struct {
    BYTE    Name[SIZEOF_SHORT_NAME];
    union {
            DWORD   PhysicalAddress;
            DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
} SECTION_HEADER, *P_SECTION_HEADER;

typedef struct {
    DWORD   VirtualAddress;
    DWORD   Size;
} DATA_DIRECTORY, *P_DATA_DIRECTORY;

#define NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct {
    WORD    Magic;
    BYTE    MajorLinkerVersion;
    BYTE    MinorLinkerVersion;
    DWORD   SizeOfCode;
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;
    DWORD   BaseOfCode;
    DWORD   BaseOfData;
    DWORD   ImageBase;
    DWORD   SectionAlignment;
    DWORD   FileAlignment;
    WORD    MajorOperatingSystemVersion;
    WORD    MinorOperatingSystemVersion;
    WORD    MajorImageVersion;
    WORD    MinorImageVersion;
    WORD    MajorSubsystemVersion;
    WORD    MinorSubsystemVersion;
    DWORD   Win32VersionValue;
    DWORD   SizeOfImage;
    DWORD   SizeOfHeaders;
    DWORD   CheckSum;
    WORD    Subsystem;
    WORD    DllCharacteristics;
    DWORD   SizeOfStackReserve;
    DWORD   SizeOfStackCommit;
    DWORD   SizeOfHeapReserve;
    DWORD   SizeOfHeapCommit;
    DWORD   LoaderFlags;
    DWORD   NumberOfRvaAndSizes;
    DATA_DIRECTORY DataDirectory[NUMBEROF_DIRECTORY_ENTRIES];
} OPTIONAL_HEADER, *P_OPTIONAL_HEADER;

typedef struct {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   Name;
    DWORD   Base;
    DWORD   NumberOfFunctions;
    DWORD   NumberOfNames;
    DWORD   AddressOfFunctions;     // RVA from base of image
    DWORD   AddressOfNames;         // RVA from base of image
    DWORD   AddressOfNameOrdinals;  // RVA from base of image
} EXPORT_DIRECTORY, *P_EXPORT_DIRECTORY;


void FgetString(char *str, FILE *bfp)
{
   char ch;

   while ((ch = fgetc(bfp)) != EOF)
   {
      *str++ = ch;
      if (ch == 0)
         break;
   }
}


void FreeNameFuncGlobals(void)
{
   if (p_Ordinals)
      free(p_Ordinals);
   if (p_Functions)
      free(p_Functions);
   if (p_Names)
      free(p_Names);

   for (int i=0; i < num_ordinals; i++)
   {
      if (p_FunctionNames[i])
         free(p_FunctionNames[i]);
   }
}

void getMSVCName(char *out_name, char *in_name)
{
   char *pos;

   if (in_name[0] == '?')  // is this a MSVC C++ mangled name?
   {
      if ((pos = strstr(in_name, "@@")) != NULL)
      {
         int len = pos - in_name;

         strcpy(out_name, &in_name[1]);  // strip off the leading '?'
         out_name[len-1] = 0;  // terminate string at the "@@"

         return;
      }
   }

   strcpy(out_name, in_name);
}

void LoadSymbols(char *filename)
{
   FILE *bfp;
   DOS_HEADER dos_header;
   LONG nt_signature;
   PE_HEADER pe_header;
   SECTION_HEADER section_header;
   BOOL edata_found;
   OPTIONAL_HEADER optional_header;
   LONG edata_offset;
   LONG edata_delta;
   EXPORT_DIRECTORY export_directory;
   LONG name_offset;
   LONG ordinal_offset;
   LONG function_offset;
   char function_name[256];
   int i, index;
   BOOL error;
   char msg[80];

   for (i=0; i < num_ordinals; i++)
      p_FunctionNames[i] = NULL;

   if ((bfp=fopen(filename, "rb")) == NULL)
   {
      sprintf(msg, "DLL file %s not found!", filename);
      ALERT(at_error, msg);
      return;
   }

   if (fread(&dos_header, sizeof(dos_header), 1, bfp) != 1)
   {
      sprintf(msg, "%s is NOT a valid DLL file!", filename);
      ALERT(at_error, msg);
      return;
   }

   if (dos_header.e_magic != DOS_SIGNATURE)
   {
      ALERT(at_error, "file does not have a valid DLL signature!");
      return;
   }

   if (fseek(bfp, dos_header.e_lfanew, SEEK_SET) == -1)
   {
      ALERT(at_error, "error seeking to new exe header!");
      return;
   }

   if (fread(&nt_signature, sizeof(nt_signature), 1, bfp) != 1)
   {
      ALERT(at_error, "file does not have a valid NT signature!");
      return;
   }

   if (nt_signature != NT_SIGNATURE)
   {
      ALERT(at_error, "file does not have a valid NT signature!");
      return;
   }

   if (fread(&pe_header, sizeof(pe_header), 1, bfp) != 1)
   {
      ALERT(at_error, "file does not have a valid PE header!");
      return;
   }

   if (pe_header.SizeOfOptionalHeader == 0)
   {
      ALERT(at_error, "file does not have an optional header!");
      return;
   }

   if (fread(&optional_header, sizeof(optional_header), 1, bfp) != 1)
   {
      ALERT(at_error, "file does not have a valid optional header!");
      return;
   }

   edata_found = FALSE;

   for (i=0; i < pe_header.NumberOfSections; i++)
   {
      if (fread(&section_header, sizeof(section_header), 1, bfp) != 1)
      {
         ALERT(at_error, "error reading section header!");
         return;
      }

      if (strcmp((char *)section_header.Name, ".edata") == 0)
      {
         edata_found = TRUE;
         break;
      }
   }

   if (edata_found)
   {
      edata_offset = section_header.PointerToRawData;
      edata_delta = section_header.VirtualAddress - section_header.PointerToRawData; 
   }
   else
   {
      edata_offset = optional_header.DataDirectory[0].VirtualAddress;
      edata_delta = 0L;
   }


   if (fseek(bfp, edata_offset, SEEK_SET) == -1)
   {
      ALERT(at_error, "file does not have a valid exports section!");
      return;
   }

   if (fread(&export_directory, sizeof(export_directory), 1, bfp) != 1)
   {
      ALERT(at_error, "file does not have a valid optional header!");
      return;
   }

   num_ordinals = export_directory.NumberOfNames;  // also number of ordinals

   if (num_ordinals > MAX_SYMBOLS)
   {
      ALERT(at_error, "too many symbols to process.  make MAX_SYMBOLS bigger.");
      return;
   }


   ordinal_offset = export_directory.AddressOfNameOrdinals - edata_delta;

   if (fseek(bfp, ordinal_offset, SEEK_SET) == -1)
   {
      ALERT(at_error, "file does not have a valid ordinals section!");
      return;
   }

   if ((p_Ordinals = (WORD *)malloc(num_ordinals * sizeof(WORD))) == NULL)
   {
      ALERT(at_error, "error allocating memory for ordinals section!");
      return;
   }

   if (fread(p_Ordinals, num_ordinals * sizeof(WORD), 1, bfp) != 1)
   {
      FreeNameFuncGlobals();

      ALERT(at_error, "error reading ordinals table!");
      return;
   }


   function_offset = export_directory.AddressOfFunctions - edata_delta;

   if (fseek(bfp, function_offset, SEEK_SET) == -1)
   {
      ALERT(at_error, "file does not have a valid export address section!");
      return;
   }

   if ((p_Functions = (DWORD *)malloc(num_ordinals * sizeof(DWORD))) == NULL)
   {
      ALERT(at_error, "error allocating memory for export address section!");
      return;
   }

   if (fread(p_Functions, num_ordinals * sizeof(DWORD), 1, bfp) != 1)
   {
      FreeNameFuncGlobals();

      ALERT(at_error, "error reading export address section!");
      return;
   }


   name_offset = export_directory.AddressOfNames - edata_delta;

   if (fseek(bfp, name_offset, SEEK_SET) == -1)
   {
      FreeNameFuncGlobals();

      ALERT(at_error, "file does not have a valid names section!");
      return;
   }

   if ((p_Names = (DWORD *)malloc(num_ordinals * sizeof(DWORD))) == NULL)
   {
      FreeNameFuncGlobals();

      ALERT(at_error, "error allocating memory for names section!");
      return;
   }

   if (fread(p_Names, num_ordinals * sizeof(DWORD), 1, bfp) != 1)
   {
      FreeNameFuncGlobals();

      ALERT(at_error, "error reading names table!");
      return;
   }

   error = FALSE;

   for (i=0; (i < num_ordinals) && (error==FALSE); i++)
   {
      name_offset = p_Names[i] - edata_delta;

      if (name_offset != 0)
      {
         if (fseek(bfp, name_offset, SEEK_SET) == -1)
            error = TRUE;
         else
         {
            FgetString(function_name, bfp);

            if ((p_FunctionNames[i] = (char *)malloc(strlen(function_name)+1)) == NULL)
               error = TRUE;
            else
               getMSVCName(p_FunctionNames[i], function_name);
         }
      }
   }

   if (error)
   {
      FreeNameFuncGlobals();

      ALERT(at_error, "error in loading names section!");
      return;
   }

   fclose(bfp);

   void *game_GiveFnptrsToDll;

   for (i=0; i < num_ordinals; i++)
   {
      if (strcmp("GiveFnptrsToDll", p_FunctionNames[i]) == 0)
      {
         index = p_Ordinals[i];

         game_GiveFnptrsToDll = (void *)GetProcAddress(h_Library, "GiveFnptrsToDll");
         base_offset = (unsigned long)(game_GiveFnptrsToDll) - p_Functions[index];

         break;
      }
   }
}


unsigned long FUNCTION_FROM_NAME(const char *pName)
{
   int i, index;

   for (i=0; i < num_ordinals; i++)
   {
      if (strcmp(pName, p_FunctionNames[i]) == 0)
      {
         index = p_Ordinals[i];

         return p_Functions[index] + base_offset;
      }
   }

   return 0L;  // couldn't find the function name to return address
}


const char *NAME_FOR_FUNCTION(unsigned long function)
{
   int i, index;

   for (i=0; i < num_ordinals; i++)
   {
      index = p_Ordinals[i];

      if ((function - base_offset) == p_Functions[index])
      {
         return p_FunctionNames[i];
      }
   }

   return NULL;  // couldn't find the function address to return name
}
