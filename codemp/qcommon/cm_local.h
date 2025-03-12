/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

#include "cm_polylib.h"
#include "cm_public.h"
#include "qcommon/qcommon.h"

#define	MAX_SUBMODELS			512
#define	BOX_MODEL_HANDLE		(MAX_SUBMODELS-1)
#define CAPSULE_MODEL_HANDLE	(MAX_SUBMODELS-2)

// Define cm_float - floating point type used for various intermediate
// calculations in collision map subsystem.
typedef float cm_float;
typedef float cm_vec3_t[3];

static inline cm_float CM_sqrt(cm_float x) {
	return sqrtf(x);
}

static inline cm_float CM_sin(cm_float x) {
	return sinf(x);
}

static inline cm_float CM_cos(cm_float x) {
	return cosf(x);
}

// CM Math functions
static inline cm_float CM_DotProduct( const vec3_t vec1, const vec3_t vec2 ) {
	return (cm_float)vec1[0]*vec2[0] + (cm_float)vec1[1]*vec2[1] + (cm_float)vec1[2]*vec2[2];
}

static inline cm_float CM_VectorLengthSquared( const vec3_t vec ) {
	return (cm_float)vec[0]*vec[0] + (cm_float)vec[1]*vec[1] + (cm_float)vec[2]*vec[2];
}

static inline void CM_VectorMA( const vec3_t vec1, cm_float scale, const vec3_t vec2, vec3_t vecOut ) {
	vecOut[0] = vec1[0] + scale*vec2[0];
	vecOut[1] = vec1[1] + scale*vec2[1];
	vecOut[2] = vec1[2] + scale*vec2[2];
}

static inline void CM_VectorScale( const vec3_t vecIn, cm_float scale, vec3_t vecOut ) {
	vecOut[0] = vecIn[0]*scale;
	vecOut[1] = vecIn[1]*scale;
	vecOut[2] = vecIn[2]*scale;
}


static inline cm_float CM_VectorNormalize( vec3_t vec )
{
	float	length, ilength;

	length = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
	length = sqrtf( length );

	if ( length ) {
		ilength = 1/length;
		vec[0] *= ilength;
		vec[1] *= ilength;
		vec[2] *= ilength;
	}

	return length;
}

static inline void CM_AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	cm_float		angle;
	static cm_float	sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = CM_sin(angle);
	cy = CM_cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = CM_sin(angle);
	cp = CM_cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = CM_sin(angle);
	cr = CM_cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}


struct Point
{
	long x, y;
};

typedef struct cNode_s {
	cplane_t	*plane;
	int			children[2];		// negative numbers are leafs
} cNode_t;

typedef struct cLeaf_s {
	int			cluster;
	int			area;

	ptrdiff_t	firstLeafBrush;
	int			numLeafBrushes;

	ptrdiff_t	firstLeafSurface;
	int			numLeafSurfaces;
} cLeaf_t;

typedef struct cmodel_s {
	vec3_t		mins, maxs;
	cLeaf_t		leaf;			// submodels don't reference the main tree
	int			firstNode;		// only for cmodel[0] (for the main and bsp instances)
} cmodel_t;

typedef struct cbrushside_s {
	cplane_t	*plane;
	int			shaderNum;
} cbrushside_t;

typedef struct cbrush_s {
	int					shaderNum;		// the shader that determined the contents
	int					contents;
	vec3_t				bounds[2];
	cbrushside_t		*sides;
	unsigned short		numsides;
	unsigned short		checkcount;		// to avoid repeated testings
} cbrush_t;

class CCMShader
{
public:
	char		shader[MAX_QPATH];
	class CCMShader	*mNext;
	int			surfaceFlags;
	int			contentFlags;

	const char	*GetName(void) const { return(shader); }
	class CCMShader *GetNext(void) const { return(mNext); }
	void	SetNext(class CCMShader *next) { mNext = next; }
	void	Destroy(void) { }
};

typedef struct cPatch_s {
	int			checkcount;				// to avoid repeated testings
	int			surfaceFlags;
	int			contents;
	struct patchCollide_s	*pc;
} cPatch_t;


typedef struct cArea_s {
	int			floodnum;
	int			floodvalid;
} cArea_t;

typedef struct clipMap_s {
	char		name[MAX_QPATH];

	int			numShaders;
	CCMShader	*shaders;

	int			numBrushSides;
	cbrushside_t *brushsides;

	int			numPlanes;
	cplane_t	*planes;

	int			numNodes;
	cNode_t		*nodes;

	int			numLeafs;
	cLeaf_t		*leafs;

	int			numLeafBrushes;
	int			*leafbrushes;

	int			numLeafSurfaces;
	int			*leafsurfaces;

	int			numSubModels;
	cmodel_t	*cmodels;

	int			numBrushes;
	cbrush_t	*brushes;

	int			numClusters;
	int			clusterBytes;
	byte		*visibility;
	qboolean	vised;			// if false, visibility is just a single cluster of ffs

	int			numEntityChars;
	char		*entityString;

	int			numAreas;
	cArea_t		*areas;
	int			*areaPortals;	// [ numAreas*numAreas ] reference counts

	int			numSurfaces;
	cPatch_t	**surfaces;			// non-patches will be NULL

	int			floodvalid;
	int			checkcount;					// incremented on each trace
} clipMap_t;


// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define	SURFACE_CLIP_EPSILON	(0.125)

extern	clipMap_t	cmg; //rwwRMG - changed from cm
extern	int			c_pointcontents;
extern	int			c_traces, c_brush_traces, c_patch_traces;
extern	cvar_t		*cm_noAreas;
extern	cvar_t		*cm_noCurves;
extern	cvar_t		*cm_playerCurveClip;
extern	cvar_t		*cm_extraVerbose;

// cm_test.c

// Used for oriented capsule collision detection
typedef struct sphere_s {
	qboolean	use;
	float		radius;
	float		halfheight;
	vec3_t		offset;
} sphere_t;

typedef struct traceWork_s { //rwwRMG - modified
	vec3_t		start;
	vec3_t		end;
	vec3_t		size[2];	// size of the box being swept through the model
	vec3_t		offsets[8];	// [signbits][x] = either size[0][x] or size[1][x]
	float		maxOffset;	// longest corner length from origin
	vec3_t		extents;	// greatest of abs(size[0]) and abs(size[1])
	vec3_t		modelOrigin;// origin of the model tracing through
	int			contents;	// ored contents of the model tracing through
	qboolean	isPoint;	// optimized case
//	trace_t		trace;		// returned from trace call
	sphere_t	sphere;		// sphere for oriendted capsule collision

	//rwwRMG - added:
	vec3pair_t		bounds;			// enclosing box of start and end surrounding by size
	vec3pair_t		localBounds;	// enclosing box of start and end surrounding by size for a segment

	float			baseEnterFrac;	// global enter fraction (before processing subsections of the brush)
	float			baseLeaveFrac;	// global leave fraction (before processing subsections of the brush)
	float			enterFrac;		// fraction where the ray enters the brush
	float			leaveFrac;		// fraction where the ray leaves the brush
	cbrushside_t	*leadside;
	cplane_t		*clipplane;
	bool			startout;
	bool			getout;

} traceWork_t;

typedef struct leafList_s {
	int		count;
	int		maxcount;
	qboolean	overflowed;
	int		*list;
	vec3_t	bounds[2];
	int		lastLeaf;		// for overflows where each leaf can't be stored individually
	void	(*storeLeafs)( struct leafList_s *ll, int nodenum );
} leafList_t;

void CM_StoreLeafs( leafList_t *ll, int nodenum );
void CM_StoreBrushes( leafList_t *ll, int nodenum );

void CM_BoxLeafnums_r( leafList_t *ll, int nodenum );

cmodel_t	*CM_ClipHandleToModel( clipHandle_t handle, clipMap_t **clipMap = 0 );

// cm_patch.c

struct patchCollide_s	*CM_GeneratePatchCollide( int width, int height, vec3_t *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, trace_t &trace, const struct patchCollide_s *pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
void CM_ClearLevelPatches( void );

// cm_shader.cpp
void CM_SetupShaderProperties( void );
void CM_ShutdownShaderProperties(void);
CCMShader *CM_GetShaderInfo( const char *name );
CCMShader *CM_GetShaderInfo( int shaderNum );
void		CM_GetModelFormalName ( const char* model, const char* skin, char* name, int size );

// cm_load.cpp
void CM_GetWorldBounds ( vec3_t mins, vec3_t maxs );
