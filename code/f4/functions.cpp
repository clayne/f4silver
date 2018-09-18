/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

//IMPORTANT(adm244): SCRATCH VERSION JUST TO GET IT UP WORKING

/*
  Compile as x64 code
*/

/*
  ConsolePrint buffer size: 4095 bytes allocated on stack
*/

/*
  Get cell regions:
    1) Call TESCell__sub_1403B4A10
    2) [eax + 0x8] - first region
    3) [eax + 0x10] - pointer to RegionUnk struct
    
      struct RegionUnk {
        TESRegion *region;
        RegionUnk *nextRegionUnk;
      };
*/

/*
  Check if cell is within region border:
    1) Get cell regions
    2) If there's border region the cell is within border region,
      otherwise it is outside.
*/

/*
  Is current cell interior or exterior check:
    1) Get ObjectReference pointer (rax)
      player is an object reference
      we have an address of player objectreference pointer (0x05AC26F8 for 1_10_26)
    2) Get parent cell ([rax + 0xB8] = rax')
    3) Check cell flags ([rax' + 0x40])
      first bit is interior\exterior (set - interior)
  END
*/

/* DEPRECATED
  Script compile and run process:
    1) Allocate memory for Script object
    2) Initialize Script object
      2.1) Set vtable and flags
      2.2) Initialize the rest of a structure with zero
      2.3) Initialize script text at field70 (260 bytes long) with zeros (optional?)
    3) Allocate memory for script text
    4) Initialize memory for script text with zeros
    5) Copy script text from Script object into allocated memory
    6) Set scriptText pointer to point to allocated memory
    
    7) COMPILE: Pass globalObject, Script object, 0 and 1 into sub_004E7B10
    8) RUN: Pass Script object, 0, 0, 0 and 1 into sub_004E2440
  END
*/

/*
  BGSSaveLoadGame:
  
    // BGSSaveLoadGame:
    //  0x05A916D0 (1_10_26)
    //  0x???????? (1_10_40)
    // filename - save file name
    // unk2 - ???
    // unk3 - buffer?
    // address:
    //  0x00CDCA40 (1_10_26)
    //  0x00CDE390 (1_10_40)
    bool LoadGame(int64 BGSSaveLoadGame, char *filename, int32 unk2, int64 unk3);
    
    // 0x00CDC540 (1_10_26)
    // 0x00CDDE90 (1_10_40)
    bool SaveGame(void *BGSSaveLoadGame, char *filename, uint32 unk2);
*/

/*
  Console command "CenterOnExterior" algorithm:
    1) Takes first WorldSpace from list of all worldspaces (stored in array)
    2) Searches for cell by given coordinates
    3) Moves player to that cell
  END
*/

/*
  GetWorldSpaceArray,
  GetWorldSpaceCount,
  GetFormArray,
  GetFormCount:
  
    rcx = [0x058ED480] (1_10_26)
    dl = 0x4A (form type)
    
    rax = dl + dl*2 (= 0xDE)
      rcx + rax*8 + 0x68 (pointer to array of worldspaces)
      rcx + rax*8 + 0x78 (total worldspace count)  
*/

#ifndef _F4_FUNCTIONS_
#define _F4_FUNCTIONS_

#include <string.h>

#include "native_functions.h"
#include "signatures.cpp"

#define MAX_SCRIPT_SIZE 16384
#define MAX_SCRIPT_LINE 260
#define VM_OPCODE_LENGTH 4

internal const int DefaultCompiler = 0;
internal const int SysWindowCompileAndRun = 1;
internal const int DialogueCompileAndRun = 2;

// ------ Functions ------
internal bool TES_ExecuteScriptLine(char *text)
{
  bool result = false;
  
  TESScript scriptObject = {0};
  
  TESScript_Constructor(&scriptObject);
  TESScript_MarkAsTemporary(&scriptObject);
  TESScript_SetText(&scriptObject, text);
  result = TESScript_CompileAndRun(&scriptObject, *(void **)(GlobalScriptStateAddress), SysWindowCompileAndRun, 0);
  TESScript_Destructor(&scriptObject);
  
  return result;
}

internal inline TESPlayer * TES_GetPlayer()
{
  return *(TESPlayer **)(PlayerReferenceAddress);
}

internal inline bool TES_IsInterior(TESCell *cell)
{
  return cell->flags & FLAG_TESCell_IsInterior;
}

internal inline bool TES_IsInInterior(TESObjectReference *ref)
{
  bool result = false;

  if( ref ) {
    TESCell *parentCell = ref->parentCell;
    if( parentCell ) result = TES_IsInterior(parentCell);
  }
  
  return result;
}

internal inline TESLocation * TES_GetPlayerLocation()
{
  TESPlayer *player = TES_GetPlayer();
  return player->location;
}

//FIX(adm244): spell worldspace in types.h with non-capital 's'
internal TESWorldSpace * GetPlayerCurrentWorldSpace()
{
  TESWorldSpace *worldspace = 0;

  TESPlayer *player = TES_GetPlayer();
  TESCell *playerCell = player->tesActor.objectReference.parentCell;
  
  if( playerCell ) {
    worldspace = playerCell->worldSpace;
    
    if( worldspace ) {
      while( worldspace->parentWorldSpace ) {
        worldspace = worldspace->parentWorldSpace;
      }
    } else {
      TESLocation *location = TES_GetPlayerLocation();
      if( location ) {
        while( location->parent ) {
          location = location->parent;
        }
        
        int worldSpaceCount = TES_GetWorldSpaceCount();
        TESWorldSpace **worldspaceArray = TES_GetWorldSpaceArray();
        
        for( int i = 0; i < worldSpaceCount; ++i ) {
          //NOTE(adm244): convert it to be syntactically an array?
          TESWorldSpace *p = *(worldspaceArray + i);
          
          if( p->location == location ) {
            //NOTE(adm244): get root worldspace?
            worldspace = p;
            break;
          }
        }
        
        if( !worldspace ) {
          //NOTE(adm244): location isn't attached to any of worldspaces, try encounter zone?
        }
      } else {
        //NOTE(adm244): no location attached to player's cell, try encounter zone?
      }
    }
  }
  
  return worldspace;
}

internal bool IsCellWithinBorderRegion(TESCell *cell)
{
  //FIX(adm244): doesn't work for every cell (editor bug)
  bool result = false;

  TESCellUnk *cellUnk = TESCell_GetUnk(cell, 1);
  
  if( cellUnk ) {
    RegionUnk *regionUnk = (RegionUnk *)(&cellUnk->region);
    if( regionUnk ) {
      while( regionUnk ) {
        TESRegion *region = regionUnk->region;
        if( !region ) break;
        
        result = ((region->tesForm.flags) >> 6) && 1;
        if( result ) break;
        
        regionUnk = regionUnk->nextRegionUnk;
      }
    }
  }
  
  return result;
}

internal bool IsInDialogueWithPlayer(TESActor *actor)
{
  //NOTE(adm244): has potential problems with some dialogues
  return actor->flags_3 & FLAG_TESActor_IsInDialogue;
}

//internal bool TES_IsTalking(TESActor *actor)
//{
  // call 0x270 member function
//}

internal bool IsInMenuMode()
{
  //TODO(adm244): get class member macro?
  //uint64 unkObject01 = *((uint64 *)UnknownObject01Address);
  uint64 unkObject01 = *((uint64 *)Unk3ObjectAddress);
  return *((uint8 *)(unkObject01 + 0x1D0)) == 1;
}

internal inline bool IsPlayerInInterior()
{
  return TES_IsInInterior((TESObjectReference *)TES_GetPlayer());
}

internal inline bool IsPlayerInDialogue()
{
  return IsInDialogueWithPlayer((TESActor *)TES_GetPlayer());
}

internal inline bool IsMenuOpen(char *str)
{
  bool result = false;

  //TODO(adm244): cache bsString
  BSFixedString *bsString;
  BSFixedString_Constructor(&bsString, str);
  result = Native_IsMenuOpen(BSInputEventReceiverPtr, &bsString);
  BSFixedString_Release(&bsString);
  
  return result;
}

internal inline bool IsActorDead(TESActor *actor)
{
  //TODO(adm244): member function call macro?
  _TESActor_IsDead funcPtr = (_TESActor_IsDead)( *(((uint64 *)((TESForm *)actor)->vtable) + 192) );
  return funcPtr(actor, 1);
}

//FIX(adm244): it's actually more like FillConsoleBackbuffer,
// because it fills a buffer and adds some meta data to it (like text length, etc.)
// which then gets copied into real console buffer
internal inline void TESConsolePrint(char *format, ...)
{
  va_list args;
  va_start(args, format);
  //FIX(adm244): TES_GetConsoleObject is actually not an object, it's a structure
  TESFillConsoleBackbufferVA(*(void **)(TESConsoleObjectAddress), format, args);
  va_end(args);
}
// ------ #Functions ------

#endif
