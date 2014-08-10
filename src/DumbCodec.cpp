/*
 *      Copyright (C) 2014 Arne Morten Kvarving
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "xbmc/libXBMC_addon.h"

extern "C" {
#include <dumb.h>
#include "xbmc/xbmc_audiodec_dll.h"
#include "xbmc/AEChannelData.h"

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  XBMC=NULL;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

struct dumbfile_mem_status
{
  const uint8_t * ptr;
  unsigned offset, size;

  dumbfile_mem_status() : offset(0), size(0), ptr(NULL) {}

  ~dumbfile_mem_status()
  {
    delete[] ptr;
  }
};

static int dumbfile_mem_skip(void * f, long n)
{
  dumbfile_mem_status * s = (dumbfile_mem_status *) f;
  s->offset += n;
  if (s->offset > s->size)
  {
    s->offset = s->size;
    return 1;
  }

  return 0;
}

static int dumbfile_mem_getc(void * f)
{
  dumbfile_mem_status * s = (dumbfile_mem_status *) f;
  if (s->offset < s->size)
  {
    return *(s->ptr + s->offset++);
  }
  return -1;
}

static long dumbfile_mem_getnc(char * ptr, long n, void * f)
{
  dumbfile_mem_status * s = (dumbfile_mem_status *) f;
  long max = s->size - s->offset;
  if (max > n) max = n;
  if (max)
  {
    memcpy(ptr, s->ptr + s->offset, max);
    s->offset += max;
  }
  return max;
}

static int dumbfile_mem_seek(void * f, long n)
{
  dumbfile_mem_status * s = (dumbfile_mem_status *) f;
  if ( n < 0 || n > s->size ) return -1;
  s->offset = n;
  return 0;
}

static long dumbfile_mem_get_size(void * f)
{
  dumbfile_mem_status * s = (dumbfile_mem_status *) f;
  return s->size;
}

static DUMBFILE_SYSTEM mem_dfs = {
  NULL, // open
  &dumbfile_mem_skip,
  &dumbfile_mem_getc,
  &dumbfile_mem_getnc,
  NULL // close
};


struct DumbContext
{
  DUH* module;
  DUH_SIGRENDERER* sr;
};


void* Init(const char* strFile, unsigned int filecache, int* channels,
           int* samplerate, int* bitspersample, int64_t* totaltime,
           int* bitrate, AEDataFormat* format, const AEChannel** channelinfo)
{
  void* file = XBMC->OpenFile(strFile,0);
  if (!file)
    return NULL;

  dumbfile_mem_status memdata;
  memdata.size = XBMC->GetFileLength(file);
  memdata.ptr = new uint8_t[memdata.size];
  XBMC->ReadFile(file, const_cast<uint8_t*>(memdata.ptr), memdata.size);
  XBMC->CloseFile(file);

  DUMBFILE* f = dumbfile_open_ex(&memdata, &mem_dfs);
  if (!f)
    return NULL;

  DumbContext* result = new DumbContext;

  if (memdata.size >= 4 &&
      memdata.ptr[0] == 'I' && memdata.ptr[1] == 'M' &&
      memdata.ptr[2] == 'P' && memdata.ptr[3] == 'M')
  {
    result->module = dumb_read_it_quick(f);
  }
  else if (memdata.size >= 17 &&
           memcmp(memdata.ptr, "Extended Module: ", 17) == 0)
  {
    result->module = dumb_read_xm_quick(f);
  }
  else if (memdata.size >= 0x30 &&
           memdata.ptr[0x2C] == 'S' && memdata.ptr[0x2D] == 'C' &&
           memdata.ptr[0x2E] == 'R' && memdata.ptr[0x2F] == 'M')
  {
    result->module = dumb_read_s3m_quick(f);
  }
  else
  {
    dumbfile_close(f);
    delete result;
    return NULL;
  }

  dumbfile_close(f);

  result->sr = duh_start_sigrenderer(result->module, 0, 2, 0);

  if (!result->sr)
  {
    delete result;
    return NULL;
  }

  *channels = 2;
  *samplerate = 48000;
  *bitspersample = 16;
  *totaltime = duh_get_length(result->module);
  *format = AE_FMT_S16NE;
   static enum AEChannel map[3] = { AE_CH_FL, AE_CH_FR , AE_CH_NULL};

  *channelinfo = map;
  *bitrate = duh_sigrenderer_get_n_channels(result->sr);

  return result;
}

int ReadPCM(void* context, uint8_t* pBuffer, int size, int *actualsize)
{
  DumbContext* dumb = (DumbContext*)context;
  if (!context)
    return 1;

  int rendered = duh_render(dumb->sr, 16, 0, 1.0,
                            65536.0/48000.0,
                            size/4,pBuffer);
  *actualsize = rendered*4;

  return 0;
}

int64_t Seek(void* context, int64_t time)
{
  return time;
}

bool DeInit(void* context)
{
  DumbContext* dumb = (DumbContext*)context;
  duh_end_sigrenderer(dumb->sr);
  unload_duh(dumb->module);
  delete dumb;

  return true;
}

bool ReadTag(const char* strFile, char* title, char* artist,
             int* length)
{
  return false;
}

int TrackCount(const char* strFile)
{
  return 1;
}
}
