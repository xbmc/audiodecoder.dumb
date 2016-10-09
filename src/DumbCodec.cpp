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

#include <kodi/audiodecoder/AudioDecoder.h>
#include <kodi/VFS.h>

extern "C" {
#include <dumb.h>

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

} /* extern "C" */

class CDumbCodecAddon
  : public kodi::addon::CInstanceAudioDecoder
{
public:
  CDumbCodecAddon(KODI_HANDLE instance);
  virtual ~CDumbCodecAddon();

  virtual bool Init(std::string file, unsigned int filecache, AUDIODEC_STREAM_INFO& streamInfo);
  virtual int ReadPCM(uint8_t* buffer, int size, int& actualsize);

private:
  DUH* m_module;
  DUH_SIGRENDERER* m_sr;;
};

CDumbCodecAddon::CDumbCodecAddon(KODI_HANDLE instance)
  : CInstanceAudioDecoder(instance)
{
}

CDumbCodecAddon::~CDumbCodecAddon()
{
  duh_end_sigrenderer(m_sr);
  unload_duh(m_module);
}

bool CDumbCodecAddon::Init(std::string filename, unsigned int filecache, AUDIODEC_STREAM_INFO& streamInfo)
{
  kodi::vfs::CFile file;
  if (!file.OpenFile(filename))
    return false;

  dumbfile_mem_status memdata;
  memdata.size = file.GetLength();
  memdata.ptr = new uint8_t[memdata.size];
  file.Read(const_cast<uint8_t*>(memdata.ptr), memdata.size);
  file.Close();

  DUMBFILE* f = dumbfile_open_ex(&memdata, &mem_dfs);
  if (!f)
    return false;

  if (memdata.size >= 4 &&
      memdata.ptr[0] == 'I' && memdata.ptr[1] == 'M' &&
      memdata.ptr[2] == 'P' && memdata.ptr[3] == 'M')
  {
    m_module = dumb_read_it(f);
  }
  else if (memdata.size >= 17 &&
           memcmp(memdata.ptr, "Extended Module: ", 17) == 0)
  {
    m_module = dumb_read_xm(f);
  }
  else if (memdata.size >= 0x30 &&
           memdata.ptr[0x2C] == 'S' && memdata.ptr[0x2D] == 'C' &&
           memdata.ptr[0x2E] == 'R' && memdata.ptr[0x2F] == 'M')
  {
    m_module = dumb_read_s3m(f);
  }
  else
  {
    dumbfile_close(f);
    return false;
  }

  dumbfile_close(f);

  m_sr = duh_start_sigrenderer(m_module, 0, 2, 0);

  if (!m_sr)
  {
    return false;
  }

  streamInfo.channels = 2;
  streamInfo.samplerate = 48000;
  streamInfo.bitspersample = 16;
  streamInfo.totaltime = duh_get_length(m_module)/65536*1000;
  streamInfo.format = AUDIO_FMT_S16NE;
  streamInfo.channellist = { AUDIO_CH_FL, AUDIO_CH_FR, AUDIO_CH_NULL };
  streamInfo.bitrate = duh_sigrenderer_get_n_channels(m_sr);

  return true;
}

int CDumbCodecAddon::ReadPCM(uint8_t* buffer, int size, int& actualsize)
{
  int rendered = duh_render(m_sr, 16, 0, 1.0,
                            65536.0/48000.0,
                            size/4,buffer);
  actualsize = rendered*4;

  return 0;
}

class CMyAddon : public ::kodi::addon::CAddonBase
{
public:
  CMyAddon() { }
  virtual ADDON_STATUS CreateInstance(int instanceType,
                                      std::string instanceID,
                                      KODI_HANDLE instance,
                                      KODI_HANDLE& addonInstance) override;
};

ADDON_STATUS CMyAddon::CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance)
{
  kodi::Log(LOG_NOTICE, "Creating Dumb Audio Decoder");
  addonInstance = new CDumbCodecAddon(instance);
  return ADDON_STATUS_OK;
}

ADDONCREATOR(CMyAddon);
