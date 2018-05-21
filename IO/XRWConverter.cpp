/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
   University of Utah.


   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

/**
  \file    XRWConverter.cpp
  \author  Toan Nguyen, MIVP
  \date    May 2018
*/

#include <fstream>
#include "XRWConverter.h"
#include <Controller/Controller.h>
#include <Basics/SysTools.h>
#include <IO/KeyValueFileParser.h>
#include <cctype>
#include "TuvokIOError.h"
#include "3rdParty/xrw/libxrw.h"

using namespace std;


XRWConverter::XRWConverter()
{
  m_vConverterDesc = "XRW s2plot format";
  m_vSupportedExt.push_back("XRW");
}

bool XRWConverter::ConvertToRAW(const std::string& strSourceFilename,
                                 const std::string& strTempDir, bool,
                                 uint64_t& iHeaderSkip,
                                 unsigned& iComponentSize,
                                 uint64_t& iComponentCount,
                                 bool& bConvertEndianess, bool& bSigned,
                                 bool& bIsFloat, UINT64VECTOR3& vVolumeSize,
                                 FLOATVECTOR3& vVolumeAspect,
                                 std::string& strTitle,
                                 std::string& strIntermediateFile,
                                 bool& bDeleteIntermediateFile)
{
  MESSAGE("Attempting to convert XRW dataset %s", strSourceFilename.c_str());

  XRAW_STRUCT *xr = loadXraw((char*)strSourceFilename.c_str());
  if (!xr) {
    WARNING("Could not open XRW file %s", strSourceFilename.c_str());
    return false;
  }
  showXraw(xr);

  /*
  int stride[3] = {1,1,1};
  VOL_STRUCT *vol = Xraw2Xvol(xr, stride);
  if (!vol) {
    WARNING("Failed to parse data volume");
    return false;
  }
  showXvol(vol);
  */

  int nx, ny, nz;
  nx = xr->nx;
  ny = xr->ny;
  nz = xr->nz;
  vVolumeSize = UINT64VECTOR3(nx,ny,nz);
  WARNING("volume is %llux%llux%llu", vVolumeSize[0], vVolumeSize[1],
          vVolumeSize[2]);
  vVolumeAspect = FLOATVECTOR3(xr->wdx, xr->wdy, xr->wdz);
  WARNING("aspect: %5.3fx%5.3fx%5.3f", vVolumeAspect[0], vVolumeAspect[1],
          vVolumeAspect[2]);

  iComponentSize = 8;
  iComponentCount = 1;
  bConvertEndianess = false;
  bSigned = false;
  bIsFloat = false;

  /*
  iHeaderSkip = static_cast<uint64_t> (3 * sizeof(int) + 3 * sizeof(float));
  WARNING("Skipping %llu bytes.", iHeaderSkip);
  strIntermediateFile = strSourceFilename;
  WARNING("Using intermediate file %s", strIntermediateFile.c_str());
  bDeleteIntermediateFile = false;
  */

  //cannot use xrw file directly because of compressed contents
  iHeaderSkip = 0;
  WARNING("Skipping %llu bytes.", iHeaderSkip);
  strIntermediateFile = strTempDir + SysTools::GetFilename(strSourceFilename) + ".temp";
  WARNING("Using intermediate file %s", strIntermediateFile.c_str());
  bDeleteIntermediateFile = false;

  LargeRAWFile RAWFile(strIntermediateFile, 0);
  RAWFile.Create();
  if (!RAWFile.IsOpen()) {
    T_ERROR("Unable to open intermediate file %s", strIntermediateFile.c_str());
    return false;
  }
  RAWFile.WriteRAW(xr->data, (uint64_t)nx*ny*nz);
  RAWFile.Close();
  WARNING("Intermediate file %s created", strIntermediateFile.c_str());

  return true;
}

bool XRWConverter::ConvertToNative(const std::string& strRawFilename, const std::string& strTargetFilename, uint64_t iHeaderSkip,
                             unsigned iComponentSize, uint64_t iComponentCount, bool bSigned, bool bFloatingPoint,
                             UINT64VECTOR3 vVolumeSize,FLOATVECTOR3 vVolumeAspect, bool bNoUserInteraction,
                             const bool bQuantizeTo8Bit) {

  return false;
}
