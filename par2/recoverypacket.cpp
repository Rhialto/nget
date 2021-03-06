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

RecoveryPacket::RecoveryPacket(void)
{
  diskfile = NULL;
  offset = 0;
  packetcontext = NULL;
}

RecoveryPacket::~RecoveryPacket(void)
{
  delete packetcontext;
}

// Load the recovery packet from disk.
//
// The header of the packet will already have been read from disk. The only
// thing that actually needs to be read is the exponent value.
// The recovery data is not read from disk at this point. Its location
// is recovered in the DataBlock object.

bool RecoveryPacket::Load(DiskFile      *_diskfile, 
                          u64            _offset, 
                          PACKET_HEADER &_header)
{
  diskfile = _diskfile;
  offset = _offset;

  // Is the packet actually large enough
  if (_header.length <= sizeof(packet))
  {
    return false;
  }

  // Save the fixed header
  packet.header = _header;

  // Set the data block to immediatly follow the header on disk
  datablock.SetLocation(diskfile, offset + sizeof(packet));
  datablock.SetLength(packet.header.length - sizeof(packet));

  // Read the rest of the packet header
  return diskfile->Read(offset + sizeof(packet.header), &packet.exponent, sizeof(packet)-sizeof(packet.header));
}
