//  This file is part of par2cmdline (a PAR 2.0 compatible file verification and
//  repair tool). See http://parchive.sourceforge.net for details of PAR 2.0.
//
//  Copyright (c) 2003 Peter Brian Clements
//
//  par2cmdline is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  par2cmdline is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "par2cmdline.h"

#ifdef _MSC_VER
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#endif

CommandLine::ExtraFile::ExtraFile(void)
: filename()
, filesize(0)
{
}

CommandLine::ExtraFile::ExtraFile(const CommandLine::ExtraFile &other)
: filename(other.filename)
, filesize(other.filesize)
{
}

CommandLine::ExtraFile& CommandLine::ExtraFile::operator=(const CommandLine::ExtraFile &other)
{
  filename = other.filename;
  filesize = other.filesize;

  return *this;
}

CommandLine::ExtraFile::ExtraFile(const string &name, u64 size)
: filename(name)
, filesize(size)
{
}


CommandLine::CommandLine(const string &filename, const vector<string> &extrafilenames, const multimap<string,string> &extrafilenamemaparg)
: parfilename(filename)
, extrafiles()
, extrafilenamemap(extrafilenamemaparg)
{
  for (vector<string>::const_iterator fni=extrafilenames.begin(); fni!=extrafilenames.end(); ++fni)
  {
    const string &filename = *fni;
    
    // All other files must exist
    if (!DiskFile::FileExists(filename))
    {
      cerr << "The source file does not exist: " << filename << endl;
      continue;
    }

    u64 filesize = DiskFile::GetFileSize(filename);

    // Ignore all 0 byte files
    if (filesize > 0)
    {
      extrafiles.push_back(ExtraFile(filename, filesize));
    }
  }
}
