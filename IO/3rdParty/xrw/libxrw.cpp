/* libxrw.c
 *
 * Copyright 2012 David G. Barnes
 *
 * This file is part of S2VOLSURF.
 *
 * S2VOLSURF is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * S2VOLSURF is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S2VOLSURF.  If not, see <http://www.gnu.org/licenses/>. 
 *
 * We would appreciate it if research outcomes using S2VOLSURF would
 * provide the following acknowledgement:
 *
 * "Three-dimensional visualisation was conducted with the S2PLOT
 * progamming library"
 *
 * and a reference to
 *
 * D.G.Barnes, C.J.Fluke, P.D.Bourke & O.T.Parry, 2006, Publications
 * of the Astronomical Society of Australia, 23(2), 82-93.
 *
 * $Id: libxrw.c 229 2014-05-13 01:35:06Z barnesd $
 *
 */

/* * * * * N.B. read/write only ints, not longs (32/64-bit diff) * * * * */

// use GZ file compression (gzopen, gzclose, ...)
#define USEGZ 1

#include "libxrw.h"
#include <stdlib.h>
#include <math.h>

#if defined(USEGZ)
#include "zlib.h"
#define CLOSEFILE(a) gzclose((a))
#else
#define CLOSEFILE(a) fclose((a))
#endif

void deleteXraw(XRAW_STRUCT *xr) {
  if (xr->data) {
    free(xr->data);
  }
  free(xr);
}

void deleteXvol(VOL_STRUCT *xv) {
  fprintf(stderr, "LIBXRW WARNING: deleteXvol TBI\n");
}

VOL_STRUCT *cloneXvol(VOL_STRUCT *vol) {
  VOL_STRUCT *ret = (VOL_STRUCT *)malloc(sizeof(VOL_STRUCT));
  strcpy(ret->filename, vol->filename);
  ret->nx = vol->nx;
  ret->ny = vol->ny;
  ret->nz = vol->nz;
  ret->wdx = vol->wdx;
  ret->wdy = vol->wdy;
  ret->wdz = vol->wdz;
  memcpy(ret->red, vol->red, 256);
  memcpy(ret->green, vol->green, 256);
  memcpy(ret->blue, vol->blue, 256);
  ret->bzero = vol->bzero;
  ret->bscale = vol->bscale;

  int i, j, k;
  ret->data = (float ***)malloc(ret->nx * sizeof(float **));
  for (i = 0; i < ret->nx; i++) {
    ret->data[i] = (float **)malloc(ret->ny * sizeof(float *));
    for (j = 0; j < ret->ny; j++) {
      ret->data[i][j] = (float *)malloc(ret->nz * sizeof(float));
      for (k = 0; k < ret->nz; k++) {
	ret->data[i][j][k] = vol->data[i][j][k];
      }
    }
  }  
  return ret;
}

void accumulateXvol(VOL_STRUCT *dest, VOL_STRUCT *src, float scale) {
  if ((dest->nx != src->nx) || (dest->ny != src->ny) || (dest->nz != src->nz)) {
    fprintf(stderr, "accumulateXvol: non-conformant volumes!\n");
    exit(-1);
  }
  int i, j, k;
  for (i = 0; i < dest->nx; i++) {
    for (j = 0; j < dest->ny; j++) {
      for (k = 0; k < dest->nz; k++) {
	dest->data[i][j][k] += scale * src->data[i][j][k];
      }
    }
  }

}

#define RAW_UINT8 1
#define RAW_UINT16 2

XRAW_STRUCT *preloadXraw(char *fname) { // just get the metadata

  // open input file
#if defined(USEGZ)
  gzFile fin = gzopen(fname, "rb");
#else
  FILE *fin = fopen(fname, "rb");
#endif
  if (!fin) {
    fprintf(stderr, "Failed to open file '%s'.\n", fname);
    return NULL;
  }

  // allocate container
  XRAW_STRUCT *xrs = (XRAW_STRUCT *)malloc(1 * sizeof(XRAW_STRUCT));
  if (!xrs) {
    fprintf(stderr, "Failed to create container for file '%s'.\n", fname);
    CLOSEFILE(fin);
    return NULL;
  }

  strcpy(xrs->filename, fname);

  // get nx, ny, nz
#if defined(USEGZ)
  if ((gzread(fin, &(xrs->nx), sizeof(xrs->nx)) != sizeof(xrs->nx)) ||
      (gzread(fin, &(xrs->ny), sizeof(xrs->ny)) != sizeof(xrs->ny)) ||
      (gzread(fin, &(xrs->nz), sizeof(xrs->nz)) != sizeof(xrs->nz))) {
#else
  if (!fread(&(xrs->nx), sizeof(xrs->nx), 1, fin) ||
      !fread(&(xrs->ny), sizeof(xrs->ny), 1, fin) || 
      !fread(&(xrs->nz), sizeof(xrs->nz), 1, fin)) {
#endif
    fprintf(stderr, "Failed to read volume dimensions.\n");
    free(xrs);
    CLOSEFILE(fin);
    return NULL;
  }

  //fprintf(stderr, "vol dimensions are %d %d %d\n", xrs->nx, xrs->ny, xrs->nz);
  //fprintf(stderr, "ftell is %ld\n", ftell(fin));

  // get wdx, wdy, wdz
#if defined(USEGZ)
  if ((gzread(fin, &(xrs->wdx), sizeof(xrs->wdx)) != sizeof(xrs->wdx)) ||
      (gzread(fin, &(xrs->wdy), sizeof(xrs->wdy)) != sizeof(xrs->wdy)) ||
      (gzread(fin, &(xrs->wdz), sizeof(xrs->wdz)) != sizeof(xrs->wdz))) {
#else
  if (!fread(&xrs->wdx, sizeof(xrs->wdx), 1, fin) ||
      !fread(&xrs->wdy, sizeof(xrs->wdy), 1, fin) || 
      !fread(&xrs->wdz, sizeof(xrs->wdz), 1, fin)) {
#endif
    fprintf(stderr, "Failed to read voxel sizes.\n");
    free(xrs);
    CLOSEFILE(fin);
    return NULL;
  }

  //fprintf(stderr, "voxels sizes are %f %f %f\n", xrs->wdx, xrs->wdy, xrs->wdz);
  //fprintf(stderr, "ftell is %ld\n", ftell(fin));

  CLOSEFILE(fin);
  return xrs;
}

XRAW_STRUCT *loadXraw(char *fname) {

  XRAW_STRUCT *xrs = preloadXraw(fname);
  if (!xrs) {
    fprintf(stderr, "Failed to preload Xraw file\n");
    return NULL;
  }

  // open input file
#if defined(USEGZ)
  gzFile fin = gzopen(fname, "rb");
#else
  FILE *fin = fopen(fname, "rb");
#endif
  if (!fin) {
    fprintf(stderr, "Failed to open file '%s'.\n", fname);
    return NULL;
  }

#if (1) 
  int offset = 3 * sizeof(xrs->nx) + 3 * sizeof(xrs->wdx);
  gzseek(fin, offset, SEEK_SET);


#else
  // allocate container
  XRAW_STRUCT *xrs = (XRAW_STRUCT *)malloc(1 * sizeof(XRAW_STRUCT));
  if (!xrs) {
    fprintf(stderr, "Failed to create container for file '%s'.\n", fname);
    CLOSEFILE(fin);
    return NULL;
  }

  strcpy(xrs->filename, fname);

  // get nx, ny, nz
#if defined(USEGZ)
  if ((gzread(fin, &(xrs->nx), sizeof(xrs->nx)) != sizeof(xrs->nx)) ||
      (gzread(fin, &(xrs->ny), sizeof(xrs->ny)) != sizeof(xrs->ny)) ||
      (gzread(fin, &(xrs->nz), sizeof(xrs->nz)) != sizeof(xrs->nz))) {
#else
  if (!fread(&(xrs->nx), sizeof(xrs->nx), 1, fin) ||
      !fread(&(xrs->ny), sizeof(xrs->ny), 1, fin) || 
      !fread(&(xrs->nz), sizeof(xrs->nz), 1, fin)) {
#endif
    fprintf(stderr, "Failed to read volume dimensions.\n");
    free(xrs);
    CLOSEFILE(fin);
    return NULL;
  }

  //fprintf(stderr, "vol dimensions are %d %d %d\n", xrs->nx, xrs->ny, xrs->nz);
  //fprintf(stderr, "ftell is %ld\n", ftell(fin));

  // get wdx, wdy, wdz
#if defined(USEGZ)
  if ((gzread(fin, &(xrs->wdx), sizeof(xrs->wdx)) != sizeof(xrs->wdx)) ||
      (gzread(fin, &(xrs->wdy), sizeof(xrs->wdy)) != sizeof(xrs->wdy)) ||
      (gzread(fin, &(xrs->wdz), sizeof(xrs->wdz)) != sizeof(xrs->wdz))) {
#else
  if (!fread(&xrs->wdx, sizeof(xrs->wdx), 1, fin) ||
      !fread(&xrs->wdy, sizeof(xrs->wdy), 1, fin) || 
      !fread(&xrs->wdz, sizeof(xrs->wdz), 1, fin)) {
#endif
    fprintf(stderr, "Failed to read voxel sizes.\n");
    free(xrs);
    CLOSEFILE(fin);
    return NULL;
  }

  //fprintf(stderr, "voxels sizes are %f %f %f\n", xrs->wdx, xrs->wdy, xrs->wdz);
  //fprintf(stderr, "ftell is %ld\n", ftell(fin));

#endif

  // get data
  long nvals = (long)xrs->nx * xrs->ny * xrs->nz;
  fprintf(stderr, "attempting to allocate %ld bytes...\n", nvals);
  //xrs->data = (unsigned char *)malloc(nvals * sizeof(unsigned char));
  xrs->data = (unsigned char *)calloc(xrs->nx * sizeof(unsigned char), xrs->ny * xrs->nz);
  if (!xrs->data) {
    fprintf(stderr, "Failed to allocate storage for volume.\n");
    free(xrs);
    CLOSEFILE(fin);
    return NULL;
  }     
  
#if defined(USEGZ)
  //if (gzread(fin, xrs->data, sizeof(unsigned char)*nvals) != sizeof(unsigned char)*nvals) {
  int _fail = 0;
  int stripe = 0;
  int stripesize = xrs->ny * xrs->nz * sizeof(unsigned char);
  while ((stripe < xrs->nx) && !_fail) {
    _fail = gzread(fin, xrs->data + (long)stripe * stripesize, stripesize) != stripesize;
    stripe++;
  }
  if (_fail) {
#else
  if (fread(xrs->data, sizeof(unsigned char), nvals, fin) != nvals) {
#endif
    fprintf(stderr, "Failed to read entire volume: inconsistent file.\n");
    free(xrs->data);
    free(xrs);
    CLOSEFILE(fin);
    return NULL;
  }
  
  //fprintf(stderr, "ftell is %ld\n", ftell(fin));
  // get look-up table components
#if defined(USEGZ)
  if ((gzread(fin, xrs->red, sizeof(unsigned char)*256) != sizeof(unsigned char)*256) ||
      (gzread(fin, xrs->green, sizeof(unsigned char)*256) != sizeof(unsigned char)*256) ||
      (gzread(fin, xrs->blue, sizeof(unsigned char)*256) != sizeof(unsigned char)*256)) {
#else
  if ((fread(xrs->red, sizeof(unsigned char), 256, fin) != 256) ||
      (fread(xrs->green, sizeof(unsigned char), 256, fin) != 256) ||
      (fread(xrs->blue, sizeof(unsigned char), 256, fin) != 256)) {
#endif
    fprintf(stderr, "Failed reading embedded colour look-up table.\n");
    //if (feof(fin)) {
    //  fprintf(stderr, "at feof of fin\n");
    //}
    free(xrs->data);
    free(xrs);
    CLOSEFILE(fin);
    return NULL;
  }

  // try to read one more byte ... this should result in feof being true
#if defined(USEGZ)
  gzread(fin, NULL, sizeof(unsigned char));
  if (!gzeof(fin)) {
#else
  fread(NULL, sizeof(unsigned char), 1, fin);
  if (!feof(fin)) {
#endif
    fprintf(stderr, "Read data, but not at end of file.  Invalid state.\n");
    free(xrs->data);
    free(xrs);
    CLOSEFILE(fin);
    return NULL;
  }

  CLOSEFILE(fin);
  return xrs;
}
  
XRAW_STRUCT *createXraw(char *fname, int nx, int ny, int nz, int borderwidth) {

  XRAW_STRUCT *xrw = (XRAW_STRUCT *)malloc(sizeof(XRAW_STRUCT));
  
  strcpy(xrw->filename, fname);

  xrw->nx = nx;
  xrw->ny = ny;
  xrw->nz = nz;
  
  xrw->wdx = xrw->wdy = xrw->wdz = 1.0;
  
  xrw->data = (unsigned char *)calloc((long)nx * (long)ny, (long)nz * sizeof(unsigned char));
  long i, j, k;
  unsigned char val, border;
  for (i = 0; i < (long)nx; i++) {
    for (j = 0; j < (long)ny; j++) {
      for (k = 0; k < (long)nz; k++) {
	val = 0;
	border = 0;
	if ((i < borderwidth) || (i > nx-borderwidth)) {
	  border++;
	}
	if ((j < borderwidth) || (j > ny-borderwidth)) {
	  border++;
	}
	if ((k < borderwidth) || (k > nz-borderwidth)) {
	  border++;
	}
	if (border == 2) {
	  val = 255;
	}
	xrw->data[k * (long)nz * (long)ny + j * (long)nx + i]  = val;
      }
    }
  }

  for (i = 0; i < 256; i++) {
    xrw->red[i] = xrw->green[i] = xrw->blue[i] = (unsigned char)i;
  }

  return xrw;

 }


XRAW_STRUCT *trimXraw(XRAW_STRUCT *xr, int trim[3]) {

  if ((2 * trim[0] > xr->nx - 2) ||
      (2 * trim[1] > xr->ny - 2) ||
      (2 * trim[2] > xr->nz - 2)) {
    fprintf(stderr, "trimXraw: cannot trim to a negative volume size\n");
    return NULL;
  } 

  XRAW_STRUCT *xro = (XRAW_STRUCT *)malloc(1 * sizeof(XRAW_STRUCT));
  if (!xro) {
    fprintf(stderr, "Failed to create container for trimmed xrw file.\n");
    return NULL;
  }

  strcpy(xro->filename, xr->filename);

  xro->nx = xr->nx - 2 * trim[0];
  xro->ny = xr->ny - 2 * trim[1];
  xro->nz = xr->nz - 2 * trim[2];

  xro->wdx = xr->wdx;
  xro->wdy = xr->wdy;
  xro->wdz = xr->wdz;

  xro->data = (unsigned char *)calloc(xro->nx * sizeof(unsigned char), xro->ny * xro->nz);
  if (!xro->data) {
    fprintf(stderr, "Failed to allocate unsigned char volume.\n");
    free(xro);
    return NULL;
  }
  long i, j, k;
  for (i = 0; i < xro->nx; i++) {
    for (j = 0; j < xro->ny; j++) {
#pragma omp parallel for private(k)
      for (k = 0; k < xro->nz; k++) {
	xro->data[k * xro->nx * xro->ny + j * xro->nx + i] = 
	  xr->data[(k+trim[2]) * xr->nx * xr->ny + (j+trim[1]) * xr->nx + (i+trim[0])];
      }
    }
  }    	   

  for (i = 0; i < 256; i++) {
    xro->red[i] = xr->red[i];
    xro->green[i] = xr->green[i];
    xro->blue[i] = xr->blue[i];
  }
  
  return xro;
}

void showXraw(XRAW_STRUCT *xr) {
  fprintf(stdout, "XRW file: '%s'\n", xr->filename);
  fprintf(stdout, "volume dimensions: %6d %6d %6d\n", xr->nx, xr->ny, xr->nz);
  fprintf(stdout, "      voxel sizes: %6.3f %6.3f %6.3f\n", xr->wdx, xr->wdy, xr->wdz);
  fprintf(stdout, "total volume size: %6.1f %6.1f %6.1f\n", xr->wdx*xr->nx, xr->wdy*xr->ny,
	  xr->wdz*xr->nz);
}


int saveXraw(XRAW_STRUCT *xr) {
  if (!xr) {
    fprintf(stderr, "Invalid xraw structure passed to saveXraw.\n");
    return -1;
  }

#if defined(USEGZ)
  gzFile fout = gzopen(xr->filename, "wb9");
#else
  FILE *fout = fopen(xr->filename, "wb");
#endif
  if (!fout) {
    fprintf(stderr, "Unable to create xraw output file.\n");
    return -1;
  }
  
  // put nx, ny, nz
#if defined(USEGZ)
  if ((gzwrite(fout, &(xr->nx), sizeof(xr->nx)) != sizeof(xr->nx)) ||
      (gzwrite(fout, &(xr->ny), sizeof(xr->ny)) != sizeof(xr->ny)) ||
      (gzwrite(fout, &(xr->nz), sizeof(xr->nz)) != sizeof(xr->nz))) {
#else
  if (!fwrite(&(xr->nx), sizeof(xr->nx), 1, fout) ||
      !fwrite(&(xr->ny), sizeof(xr->ny), 1, fout) ||
      !fwrite(&(xr->nz), sizeof(xr->nz), 1, fout)) {
#endif
    fprintf(stderr, "Failed to write volume dimensions.\n");
    CLOSEFILE(fout);
    return -1;
  }

  // put wdx, wdy, wdz
#if defined(USEGZ)
  if ((gzwrite(fout, &(xr->wdx), sizeof(xr->wdx)) != sizeof(xr->wdx)) ||
      (gzwrite(fout, &(xr->wdy), sizeof(xr->wdy)) != sizeof(xr->wdy)) ||
      (gzwrite(fout, &(xr->wdz), sizeof(xr->wdz)) != sizeof(xr->wdz))) {
#else
  if (!fwrite(&(xr->wdx), sizeof(xr->wdx), 1, fout) ||
      !fwrite(&(xr->wdy), sizeof(xr->wdy), 1, fout) ||
      !fwrite(&(xr->wdz), sizeof(xr->wdz), 1, fout)) {
#endif
    fprintf(stderr, "Failed to write voxel sizes.\n");
    CLOSEFILE(fout);
    return -1;
  }
  
  // put data
  //long nvals = xr->nx * xr->ny * xr->nz;
#if defined(USEGZ)
  //if (gzwrite(fout, xr->data, sizeof(unsigned char)*nvals) != sizeof(unsigned char)*nvals) {
  int _fail = 0;
  int stripe = 0;
  int stripesize = xr->ny * xr->nz * sizeof(unsigned char);
  while ((stripe < xr->nx) && !_fail) {
    _fail = gzwrite(fout, xr->data + (long)stripe * stripesize,  stripesize) != stripesize;
    stripe++;
  }
  if (_fail) {
#else
  long nvals = xr->nx * xr->ny * xr->nz;
  if (!fwrite(xr->data, sizeof(unsigned char), nvals, fout)) {
#endif
    fprintf(stderr, "Failed to write entire volume.\n");
    CLOSEFILE(fout);
    return -1;
  }

  // put look-up table components
#if defined(USEGZ)
  if ((gzwrite(fout, xr->red, sizeof(unsigned char)*256) != sizeof(unsigned char)*256) ||
      (gzwrite(fout, xr->green, sizeof(unsigned char)*256) != sizeof(unsigned char)*256) ||
      (gzwrite(fout, xr->blue, sizeof(unsigned char)*256) != sizeof(unsigned char)*256)) {
#else
  if (!fwrite(xr->red, sizeof(unsigned char), 256, fout) ||
      !fwrite(xr->green, sizeof(unsigned char), 256, fout) ||
      !fwrite(xr->blue, sizeof(unsigned char), 256, fout)) {
#endif
    fprintf(stderr, "Failed writing colour look-up table.\n");
    CLOSEFILE(fout);
    return -1;
  }

  CLOSEFILE(fout);
  return 0;
}

VOL_STRUCT *makePow2Xvol(VOL_STRUCT *xrv) {
  // verify provided struct
  if (!xrv || !xrv->data) {
    fprintf(stderr, "Invalid data given to makePow2Xraw.\n");
    return NULL;
  }

  // allocate container
  VOL_STRUCT *vol = (VOL_STRUCT *)malloc(1 * sizeof(VOL_STRUCT));
  if (!vol) {
    fprintf(stderr, "Failed to create parsed volume container.\n");
    return NULL;
  }

  //int w, h;
  //w = xrv->nx;
  //h = xrv->ny;

  // set filename
  strcpy(vol->filename, xrv->filename);

  // choose power-of-2 side lengths
  vol->nx = 1;
  while (vol->nx <= xrv->nx/2) {
    vol->nx *= 2;
  }
  vol->ny = 1;
  while (vol->ny <= xrv->ny/2) {
    vol->ny *= 2;
  }
  vol->nz = 1;
  while (vol->nz <= xrv->nz/2) {
    vol->nz *= 2;
  }

  // update voxel sizes
  vol->wdx = xrv->wdx * (float)xrv->nx / (float)vol->nx;
  vol->wdy = xrv->wdy * (float)xrv->ny / (float)vol->ny;
  vol->wdz = xrv->wdz * (float)xrv->nz / (float)vol->nz;
  
  // copy data with bilinear interpolation
  vol->data = (float ***)malloc(vol->nx * sizeof(float **));
  if (!vol->data) {
    fprintf(stderr,"Failed to allocate float volume.\n");
    free(vol);
    return NULL;
  }

  float xc, yc, zc; // fractional pixel in input data
  float xw1, yw1, zw1; // weights
  int i1, i2, j1, j2, k1, k2;

  int i, j, k, /* ii, ij, ik, */ ax, ay, az;
  float sum, wgt;
  for (i = 0; i < vol->nx; i++) {
    xc = (float)i * (float)xrv->nx / (float)vol->nx;
    i1 = floorf(xc);
    i2 = ceilf(xc);
    xw1 = fabs(xc-(float)i2);
    i1 = i1 < 0 ? 0 : (i1 > xrv->nx-1) ? xrv->nx-1 : i1;
    i2 = i2 < 0 ? 0 : (i2 > xrv->nx-1) ? xrv->nx-1 : i2;
    // copy this frame in to this slice
    vol->data[i] = (float **)malloc(vol->ny * sizeof(float *));
    if (!vol->data[i]) {
      fprintf(stderr, "Failed to allocate row in float volume.\n");
      return NULL;
    }
    for (j = 0; j < vol->ny; j++) {
      yc = (float)j * (float)xrv->ny / (float)vol->ny;
      j1 = floorf(yc);
      j2 = ceilf(yc);
      yw1 = fabs(yc-(float)j2);
      j1 = j1 < 0 ? 0 : (j1 > xrv->ny-1) ? xrv->ny-1 : j1;
      j2 = j2 < 0 ? 0 : (j2 > xrv->ny-1) ? xrv->ny-1 : j2;
      vol->data[i][j] = (float *)malloc(vol->nz * sizeof(float *));
      if (!vol->data[i][j]) {
	fprintf(stderr, "Failed to allocate column in float volume.\n");
	return NULL;
      }
#pragma omp parallel for private(k,zc,k1,k2,zw1,sum,wgt,ax,ay,az)
      for (k = 0; k < vol->nz; k++) {
	zc = (float)k * (float)xrv->nz / (float)vol->nz;
	k1 = floorf(zc);
	k2 = ceilf(zc);
	zw1 = fabs(zc-(float)k2);
	k1 = k1 < 0 ? 0 : (k1 > xrv->nz-1) ? xrv->nz-1 : k1;
	k2 = k2 < 0 ? 0 : (k2 > xrv->nz-1) ? xrv->nz-1 : k2;

	// averaging
	sum = 0.0;
	wgt = 0.0;
	float uxw, uyw, uzw;
	for (ax = i1; ax <= i2; ax++) {
	  uxw = xw1;
	  if (ax == i2) {
	    uxw = 1. - xw1;
	  }
	  for (ay = j1; ay <= j2; ay++) {
	    uyw = yw1;
	    if (ay == j2) {
	      uyw = 1. - yw1;
	    }
	    for (az = k1; az <= k2; az++) {
	      uzw = zw1;
	      if (az == k2) {
		uzw = 1. - zw1;
	      }
	      sum += (uxw * uyw * uzw) * xrv->data[ax][ay][az];
	      wgt += (uxw * uyw * uzw);
	    }
	  }
	}
	vol->data[i][j][k] = sum / wgt;
      }
    }
  }

  // copy look-up table
  for (i = 0; i < 256; i++) {
    vol->red[i]   = xrv->red[i];
    vol->green[i] = xrv->green[i];
    vol->blue[i]  = xrv->blue[i];
  }

  return vol;
}



VOL_STRUCT *Xraw2Xvol(XRAW_STRUCT *xrs, int stride[3]) {
  // verify loaded data
  if (!xrs || !xrs->data) {
    fprintf(stderr, "Invalid data given to parseXraw.\n");
    return NULL;
  }

  // allocate container
  VOL_STRUCT *vol = (VOL_STRUCT *)malloc(1 * sizeof(VOL_STRUCT));
  if (!vol) {
    fprintf(stderr, "Failed to create parsed volume container.\n");
    return NULL;
  }

  int w, h;
  w = xrs->nx;
  h = xrs->ny;

  strcpy(vol->filename, xrs->filename);

  // copy scalars
  vol->nx = xrs->nx / stride[0];
  vol->ny = xrs->ny / stride[1];
  vol->nz = xrs->nz / stride[2];
  vol->wdx = xrs->wdx * stride[0];
  vol->wdy = xrs->wdy * stride[1];
  vol->wdz = xrs->wdz * stride[2];

  // copy data with stride
  vol->data = (float ***)malloc(vol->nx * sizeof(float **));
  if (!vol->data) {
    fprintf(stderr,"Failed to allocate float volume.\n");
    free(vol);
    return NULL;
  }
  int i, j, k, ii, ij, ik, ax, ay, az;
  float sum;
  for (i = 0; i < vol->nx; i++) {
    ii = i * stride[0];
    // copy this frame in to this slice
    vol->data[i] = (float **)malloc(vol->ny * sizeof(float *));
    if (!vol->data[i]) {
      fprintf(stderr, "Failed to allocate row in float volume.\n");
      return NULL;
    }
    for (j = 0; j < vol->ny; j++) {
      ij = j * stride[1];
      // fill y planes in reverse direction - see below for reason
      vol->data[i][vol->ny-1-j] = (float *)malloc(vol->nz * sizeof(float *));
      if (!vol->data[i][vol->ny-1-j]) {
	fprintf(stderr, "Failed to allocate column in float volume.\n");
	return NULL;
      }
#pragma omp parallel for private(k,ik,sum,ax,ay,az)
      for (k = 0; k < vol->nz; k++) {
	ik = k * stride[2];
	// averaging
	sum = 0.0;
	for (ax = 0; ax < stride[0]; ax++) {
	  for (ay = 0; ay < stride[1]; ay++) {
	    for (az = 0; az < stride[2]; az++) {
	      sum += (float)(xrs->data[(long)(ik+az)*w*h + (ij+ay)*w + (ii+ax)]) / 255.0;
	    }
	  }
	}
	// fill y and z planes in reverse directions so that home view in 
	// s2plot (and pdf) matches orientation xrw was exported from in 
	// OsiriX
	vol->data[i][vol->ny-1-j][vol->nz-1-k] = sum / (float)(stride[0] * stride[1] * stride[2]);

      }
    }
  }

  // copy look-up table
  for (i = 0; i < 256; i++) {
    vol->red[i]   = (float)(xrs->red[i])   / 255.0;
    vol->green[i] = (float)(xrs->green[i]) / 255.0;
    vol->blue[i]  = (float)(xrs->blue[i])  / 255.0;
  }

  return vol;
}

void showXvol(VOL_STRUCT *xv) {
  fprintf(stdout, "Parsed data (from '%s'):\n", xv->filename);
  fprintf(stdout, "volume dimensions: %6d %6d %6d\n", xv->nx, xv->ny, xv->nz);
  fprintf(stdout, "      voxel sizes: %6.3f %6.3f %6.3f\n", xv->wdx, xv->wdy, xv->wdz);
  fprintf(stdout, "total volume size: %6.1f %6.1f %6.1f\n", xv->wdx*xv->nx, xv->wdy*xv->ny,
	  xv->wdz*xv->nz);
  int i, j, k;
  float v, min, max, mean, meansq;
  min = max = xv->data[0][0][0];
  mean = 0.0;
  meansq = 0.0;
  for (i = 0; i < xv->nx; i++) {
    for (j = 0; j < xv->ny; j++) {
      for (k = 0; k < xv->nz; k++) {
	v = xv->data[i][j][k];
	if (v < min) {
	  min = v;
	}
	if (v > max) {
	  max = v;
	}
	mean += v;
	meansq += v*v;
      }
    }
  }
  meansq /= (float)((float)xv->nx * xv->ny * xv->nz);
  mean /= (float)((float)xv->nx * xv->ny * xv->nz);
  float rms = sqrt(meansq - mean*mean);
  fprintf(stdout, "    min, mean(stddev), max: %6.4f %6.4f(%6.4f) %6.4f\n", 
	  min, mean, rms, max);

}


VOL_STRUCT *rebinXvol(VOL_STRUCT *ivol, int stride[3]) {
  // verify loaded data
  if (!ivol || !ivol->data) {
    fprintf(stderr, "Invalid data given to parseXraw.\n");
    return NULL;
  }

  // allocate container
  VOL_STRUCT *vol = (VOL_STRUCT *)malloc(1 * sizeof(VOL_STRUCT));
  if (!vol) {
    fprintf(stderr, "Failed to create parsed volume container.\n");
    return NULL;
  }

  //int w, h;
  //w = ivol->nx;
  //h = ivol->ny;

  strcpy(vol->filename, ivol->filename);

  // copy scalars
  vol->nx = ivol->nx / stride[0];
  vol->ny = ivol->ny / stride[1];
  vol->nz = ivol->nz / stride[2];
  vol->wdx = ivol->wdx * stride[0];
  vol->wdy = ivol->wdy * stride[1];
  vol->wdz = ivol->wdz * stride[2];

  // copy data with stride
  vol->data = (float ***)malloc(vol->nx * sizeof(float **));
  if (!vol->data) {
    fprintf(stderr,"Failed to allocate float volume.\n");
    free(vol);
    return NULL;
  }
  int i, j, k, ii, ij, ik, ax, ay, az;
  float sum;
  for (i = 0; i < vol->nx; i++) {
    ii = i * stride[0];
    // copy this frame in to this slice
    vol->data[i] = (float **)malloc(vol->ny * sizeof(float *));
    if (!vol->data[i]) {
      fprintf(stderr, "Failed to allocate row in float volume.\n");
      return NULL;
    }
    for (j = 0; j < vol->ny; j++) {
      ij = j * stride[1];
      vol->data[i][j] = (float *)malloc(vol->nz * sizeof(float *));
      if (!vol->data[i][j]) {
	fprintf(stderr, "Failed to allocate column in float volume.\n");
	return NULL;
      }
#pragma omp parallel for private(k,ik,sum,ax,ay,az)
      for (k = 0; k < vol->nz; k++) {
	ik = k * stride[2];
	// averaging
	sum = 0.0;
	for (ax = 0; ax < stride[0]; ax++) {
	  for (ay = 0; ay < stride[1]; ay++) {
	    for (az = 0; az < stride[2]; az++) {
	      sum += (float)(ivol->data[ii+ax][ij+ay][ik+az]);
	    }
	  }
	}
	vol->data[i][j][k] = sum / (float)(stride[0] * stride[1] * stride[2]);
      }
    }
  }

  // copy look-up table
  for (i = 0; i < 256; i++) {
    vol->red[i]   = ivol->red[i];
    vol->green[i] = ivol->green[i];
    vol->blue[i]  = ivol->blue[i];
  }

  return vol;
}


void normaliseXvol(VOL_STRUCT *vol) {
  int i, j, k;
  float dmin=9e99, dmax=-9e99;
  for (i = 0; i < vol->nx; i++) {
    for (j = 0; j < vol->ny; j++) {
      for (k = 0; k < vol->nz; k++) {
	if (vol->data[i][j][k] < dmin) {
	  dmin = vol->data[i][j][k];
	}
	if (vol->data[i][j][k] > dmax) {
	  dmax = vol->data[i][j][k];
	}
      }
    }
  }
  rangeNormaliseXvol(vol, dmin, dmax);
}

void rangeNormaliseXvol(VOL_STRUCT *vol, float dmin, float dmax) {
  int i, j, k;
  for (i = 0; i < vol->nx; i++) {
    for (j = 0; j < vol->ny; j++) {
#pragma omp parallel for private(k)
      for (k = 0; k < vol->nz; k++) {
	vol->data[i][j][k] = (vol->data[i][j][k] - dmin) / (dmax - dmin);
      }
    }
  }

  vol->bzero = dmin;
  vol->bscale = (dmax - dmin);
}

void tightenXvol(VOL_STRUCT *vol, float lo_sigma, float hi_sigma) {
  int i, j, k;
  float lmean = 0.0, lvar = 0.0;
  for (i = 0; i < vol->nx; i++) {
    for (j = 0; j < vol->ny; j++) {
      for (k = 0; k < vol->nz; k++) {
	lmean += vol->data[i][j][k];
	lvar += vol->data[i][j][k] * vol->data[i][j][k];
      }
    }
  }

  lmean /= (float)(vol->nx * vol->ny * vol->nz);
  lvar = lvar / (float)(vol->nx * vol->ny * vol->nz) - lmean*lmean;
  lvar = sqrt(lvar);
  float lmin = lmean + lo_sigma * lvar;
  float lmax = lmean + hi_sigma * lvar;

  for (i = 0; i < vol->nx; i++) {
    for (j = 0; j < vol->ny; j++) {
#pragma omp parallel for private(k)
      for (k = 0; k < vol->nz; k++) {
	vol->data[i][j][k] = (vol->data[i][j][k] - lmin) / (lmax - lmin);
      }
    }
  }

  vol->bzero = lmin;
  vol->bscale = (lmax - lmin);
  



 }

void derivXvol(VOL_STRUCT *vol) {

  float ***ndat = (float ***)malloc(vol->nx * sizeof(float **));

  int i, j, k;
  float dmin=9e99, dmax=-9e99;
  float dx, dy, dz;
  for (i = 0; i < vol->nx; i++) {
    ndat[i] = (float **)malloc(vol->ny * sizeof(float *));
    for (j = 0; j < vol->ny; j++) {
      ndat[i][j] = (float *)malloc(vol->nz * sizeof(float));
      for (k = 0; k < vol->nz; k++) {
	if ((i == 0) || (i == vol->nx-1) ||
	    (j == 0) || (j == vol->ny-1) ||
	    (k == 0) || (k == vol->nz-1)) {
	  ndat[i][j][k] = 0.;
	  continue;
	}
	
	dx = fabs(vol->data[i+1][j][k] - vol->data[i-1][j][k]);
	dy = fabs(vol->data[i][j+1][k] - vol->data[i][j-1][k]);
	dz = fabs(vol->data[i][j][k+1] - vol->data[i][j][k-1]);
	// normalised, so max(dx*dx+dy*dy+dz*dz) = 3

	ndat[i][j][k] = sqrt(0.333333*(dx*dx+dy*dy+dz*dz));
	

	if (ndat[i][j][k] < dmin) {
	  dmin = ndat[i][j][k];
	}
	if (ndat[i][j][k] > dmax) {
	  dmax = ndat[i][j][k];
	}
      }
    }
  }
  for (i = 0; i < vol->nx; i++) {
    for (j = 0; j < vol->ny; j++) {
#pragma omp parallel for private(k)
      for (k = 0; k < vol->nz; k++) {
	vol->data[i][j][k] = (ndat[i][j][k] - dmin) / (dmax - dmin);
      }
      free(ndat[i][j]);
    }
    free(ndat[i]);
  }

  vol->bzero = dmin;
  vol->bscale = (dmax - dmin);

}



XRAW_STRUCT *Xvol2Xraw(VOL_STRUCT *vol) {
  
  // verify Xvol data
  if (!vol || !vol->data) {
    fprintf(stderr, "Invalid data given to parseXraw.\n");
    return NULL;
  }

  // allocate container
  XRAW_STRUCT *xrs = (XRAW_STRUCT *)malloc(1 * sizeof(XRAW_STRUCT));
  if (!xrs) {
    fprintf(stderr, "Failed to create parsed volume container.\n");
    return NULL;
  }

  int w, h;
  w = vol->nx;
  h = vol->ny;

  strcpy(xrs->filename, vol->filename);

  // copy scalars
  xrs->nx = vol->nx;
  xrs->ny = vol->ny;
  xrs->nz = vol->nz;
  xrs->wdx = vol->wdx;
  xrs->wdy = vol->wdy;
  xrs->wdz = vol->wdz;

  // copy data
  //xrs->data = (unsigned char *)malloc(xrs->nx * xrs->ny * xrs->nz * sizeof(unsigned char));
  xrs->data = (unsigned char *)calloc(xrs->nx * sizeof(unsigned char), xrs->ny * xrs->nz);
  if (!xrs->data) {
    fprintf(stderr,"Failed to allocate unsigned char volume.\n");
    free(vol);
    return NULL;
  }

  int i, j, k;
  float tmp;
  for (i = 0; i < xrs->nx; i++) {
    for (j = 0; j < xrs->ny; j++) {
#pragma omp parallel for private(k,tmp)
      for (k = 0; k < xrs->nz; k++) {
	// fill y and z planes in reverse directions so that home view in 
	// s2plot (and pdf) matches orientation xrw was exported from in 
	// OsiriX
	tmp = vol->data[i][xrs->ny-1-j][xrs->nz-1-k] * 256;
	if (tmp < 0.) { tmp = 0.; }
	if (tmp > 255.999) { tmp = 255.999; }
	xrs->data[(long)k * w * h + j * w + i] = (unsigned char)(floorf(tmp));
      }
    }
  }
  
  // copy look-up table
  for (i = 0; i < 256; i++) {
    xrs->red[i]   = (unsigned char)floorf((vol->red[i]) *255.999);
    xrs->green[i] = (unsigned char)floorf((vol->green[i]) * 255.999);
    xrs->blue[i]  = (unsigned char)floorf((vol->blue[i]) * 255.999);
  }

  return xrs;
}


VOL_STRUCT *loadRaw(char *filename, int nx, int ny, int nz, int type) {
  fprintf(stderr, "LIBXRW WARNING: loadRaw NOT 64BIT COMPLIANT\n");
  VOL_STRUCT *vol = (VOL_STRUCT *)malloc(1 * sizeof(VOL_STRUCT));
  if (!vol) {
    fprintf(stderr, "Failed to create parsed volume container.\n");
    return NULL;
  }

  FILE *fin = fopen(filename, "rb");
  if (!fin) {
    fprintf(stderr, "Failed to open %s.\n", filename);
    return NULL;
  }

  vol->nx = nx;
  vol->ny = ny;
  vol->nz = nz;
  int np = nx * ny * nz;
  int i, j, k;

  switch (type) {
  case RAW_UINT8:
    {
      fprintf(stderr, "RAW_UINT8 not yet implemented!\n");
      return NULL;
      break;
    }

  case RAW_UINT16:
    {
      if (sizeof(unsigned short) != 2) {
	fprintf(stderr, "Um, no suitable storage for 16b raw!!!\n");
	return NULL;
      }
      unsigned short *id = (unsigned short *)malloc(np * sizeof(unsigned short));
      if (fread(id, sizeof(unsigned short), np, fin) != np) {
	fprintf(stderr, "failed to read correct number of data points!\n");
	return NULL;
      }
      vol->data = (float ***)malloc(nx * sizeof(float **));
      
      for (i = 0; i < nx; i++) {
	vol->data[i] = (float **)malloc(ny * sizeof(float *));
	for (j = 0; j < ny; j++) {
	  vol->data[i][j] = (float *)malloc(nz * sizeof(float));
	  for (k = 0; k < nz; k++) {
	    vol->data[i][j][k] = (float)id[i + j * nx + k * nx * ny];
	    //vol->data[i][j][k] = (float)id[i * ny * nz + j * nz + k];
	    
	    // apply specific window
	    if (vol->data[i][j][k] < 100) {
	      vol->data[i][j][k] = 0;
	    }
	    if (vol->data[i][j][k] > 1500) {
	      vol->data[i][j][k] = 1500;
	    }
	  }
	}
      }
      break;
    }

  default:
    {
      fprintf(stderr, "Unknown RAW format.\n");
      return NULL;
    }
  }

  vol->wdx = vol->wdy = vol->wdz = 1.0;
  for (i = 0; i < 256; i++) {
    vol->red[i] = vol->green[i] = vol->blue[i] = (float)i / 255.;
  }

  strcpy(vol->filename, filename);

  return vol;
}
