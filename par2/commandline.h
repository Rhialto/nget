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

#ifndef __COMMANDLINE_H__
#define __COMMANDLINE_H__

// The CommandLine object is responsible for understanding the format
// of the command line parameters are parsing the command line to
// extract details as to what the user wants to do.

class CommandLine
{
public:
  CommandLine(const string &filename, const vector<string> &extrafilenames, const multimap<string,string> &extrafilenamemap);

  // Any extra files listed on the command line
  class ExtraFile
  {
  public:
    ExtraFile(void);
    ExtraFile(const ExtraFile&);
    ExtraFile& operator=(const ExtraFile&);

    ExtraFile(const string &name, u64 size);

    string FileName(void) const {return filename;}
    u64 FileSize(void) const {return filesize;}

  protected:
    string filename;
    u64    filesize;
  };

public:
  // Accessor functions for the command line parameters

  string                              GetParFilename(void) const {return parfilename;}
  const list<CommandLine::ExtraFile>& GetExtraFiles(void) const  {return extrafiles;}
  const multimap<string,string>& GetExtraFilenameMap(void) const {return extrafilenamemap;}

protected:
  string parfilename;          // The name of the PAR2 file to create, or
                               // the name of the first PAR2 file to read
                               // when verifying or repairing.

  list<ExtraFile> extrafiles;  // The list of other files specified on the
                               // command line. When creating, this will be
                               // the source files, and when verifying or
                               // repairing, this will be additional PAR2
                               // files or data files to be examined.

  const multimap<string,string> &extrafilenamemap; // Maps source file names to alternate names they might be found under.
};

typedef list<CommandLine::ExtraFile>::const_iterator ExtraFileIterator;

#endif // __COMMANDLINE_H__
