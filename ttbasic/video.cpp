#include "video_driver.h"

bool Video::allocBacking(int w, int h, int &x, int &y)
{
  Rect r = m_bin.Insert(w, h, false, GuillotineBinPack::RectBestAreaFit, GuillotineBinPack::Split256);
  x = r.x; y = r.y + m_current_mode.y;
  return r.height != 0;
}

void Video::freeBacking(int x, int y, int w, int h)
{
  Rect r;
  r.x = x; r.y = y - m_current_mode.y; r.width = w; r.height = h;
  m_bin.Free(r, true);
}

void Video::reset()
{
  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);
}
