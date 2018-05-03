// Author: Glenn Chun
// Date: April 16, 2018

#include <math.h>
#include <stdbool.h>

#define WASM_EXPORT __attribute__((visibility("default")))

// The bounding box of the undeformed lattice.
double mBBoxMinX, mBBoxMinY, mBBoxMinZ;
double mBBoxMaxX, mBBoxMaxY, mBBoxMaxZ;

// Number of spans in each parameter direction, S/T/U.
int mSpanCounts[ 3 ];

// Number of control points in each parameter direction, S/T/U.
int mCtrlPtCounts[ 3 ];

// Total number of control points.
// Hard-coded for WebAssembly experiment.
int mTotalCtrlPtCount = 27;

// S/T/U axes.
double mAxes[ 3 ][ 3 ];

// Positions of all control points.
// 27: Hard-coded for WebAssembly experiment.
double mCtrlPts[ 27 ][ 3 ];

// The number of vertices of TorusKnotGeometry model when subdivision level is 1.
const int mModelVertCount = 2048;
double mModelVertsX[ 2048 ];
double mModelVertsY[ 2048 ];
double mModelVertsZ[ 2048 ];
double mModelVertsUndeformedX[ 2048 ];
double mModelVertsUndeformedY[ 2048 ];
double mModelVertsUndeformedZ[ 2048 ];

// On-volume points
const int mVolumeSpanCount[ 3 ] = { 16, 16, 16 };
// 16 + 1 = 17
const int mVolumePointCount[ 3 ] = { 17, 17, 17 };
// 17 * 17 * 17 = 4913
const int mVolumePointCountTotal = 4913;
double mVolumePointsX[ 4913 ];
double mVolumePointsY[ 4913 ];
double mVolumePointsZ[ 4913 ];

// Returns the bounding box of the undeformed lattice.
double bboxMinX() { return mBBoxMinX; }
double bboxMinY() { return mBBoxMinY; }
double bboxMinZ() { return mBBoxMinZ; }
double bboxMaxX() { return mBBoxMaxX; }
double bboxMaxY() { return mBBoxMaxY; }
double bboxMaxZ() { return mBBoxMaxZ; }

// Returns the number of control points on the given parameter direction.
// direction: 0 for S, 1 for T, and 2 for U.
WASM_EXPORT
int getCtrlPtCount( int direction ) { return mCtrlPtCounts[ direction ]; }

// Returns the total number of control points.
WASM_EXPORT
int getTotalCtrlPtCount() { return mTotalCtrlPtCount; }

// Converts the given ternary index to a unary index.
WASM_EXPORT
int getIndex( int i, int j, int k ) {
	return i * mCtrlPtCounts[ 1 ] * mCtrlPtCounts[ 2 ] + j * mCtrlPtCounts[ 2 ] + k;
}

// Returns the position of the control point at the given unary index.
double *getPosition( int index ) { return mCtrlPts[ index ]; }

WASM_EXPORT
double getPositionX( int index ) { return mCtrlPts[ index ][ 0 ]; }
WASM_EXPORT
double getPositionY( int index ) { return mCtrlPts[ index ][ 1 ]; }
WASM_EXPORT
double getPositionZ( int index ) { return mCtrlPts[ index ][ 2 ]; }

// Sets the position of the control point at the given unary index.
WASM_EXPORT
void setPosition( int index, double pt_x, double pt_y, double pt_z ) {
	mCtrlPts[ index ][ 0 ] = pt_x;
	mCtrlPts[ index ][ 1 ] = pt_y;
	mCtrlPts[ index ][ 2 ] = pt_z;
}

// Returns the position of the control point at the given ternary index.
double *getPositionTernary( int i, int j, int k ) {
	return getPosition( getIndex( i, j, k ) );
}

// Sets the position of the control point at the given ternary index.
void setPositionTernary( int i, int j, int k, double pt_x, double pt_y, double pt_z ) {
	setPosition( getIndex( i, j, k ), pt_x, pt_y, pt_z );
}

// Returns n-factorial.
int facto( int n ) {
	int fac = 1;
	for( int i = n; i > 0; i-- )
		fac *= i;
	return fac;
}

// Returns the Bernstein polynomial in one parameter, u.
double bernstein( int n, int k, double u ) {
	// Binomial coefficient
	double coeff = facto( n ) / ( double ) ( facto( k ) * facto( n - k ) );
	return coeff * pow( 1 - u, n - k ) * pow( u, k );
}

// Rebuilds the lattice with new control points.
WASM_EXPORT
void rebuildLattice(
	double bboxMinX, double bboxMinY, double bboxMinZ,
	double bboxMaxX, double bboxMaxY, double bboxMaxZ,
	int spanCountX, int spanCountY, int spanCountZ )
{
	//// Do not rebuild the lattice if the bounding box and span counts are the same as before.
	//if( mBBoxMinX == bboxMinX && mBBoxMinY == bboxMinY && mBBoxMinZ == bboxMinZ &&
	//	mBBoxMaxX == bboxMaxX && mBBoxMaxY == bboxMaxY && mBBoxMaxZ == bboxMaxZ &&
	//	mSpanCounts[ 0 ] == spanCountX &&
	//	mSpanCounts[ 1 ] == spanCountY &&
	//	mSpanCounts[ 2 ] == spanCountZ )
	//	return;

	mBBoxMinX = bboxMinX;
	mBBoxMinY = bboxMinY;
	mBBoxMinZ = bboxMinZ;
	mBBoxMaxX = bboxMaxX;
	mBBoxMaxY = bboxMaxY;
	mBBoxMaxZ = bboxMaxZ;

	mSpanCounts[ 0 ] = spanCountX;
	mSpanCounts[ 1 ] = spanCountY;
	mSpanCounts[ 2 ] = spanCountZ;

	mCtrlPtCounts[ 0 ] = mSpanCounts[ 0 ] + 1;
	mCtrlPtCounts[ 1 ] = mSpanCounts[ 1 ] + 1;
	mCtrlPtCounts[ 2 ] = mSpanCounts[ 2 ] + 1;

	mTotalCtrlPtCount = mCtrlPtCounts[ 0 ] * mCtrlPtCounts[ 1 ] * mCtrlPtCounts[ 2 ];

	// Set the S/T/U axes.
	mAxes[ 0 ][ 0 ] = mBBoxMaxX - mBBoxMinX;
	mAxes[ 1 ][ 1 ] = mBBoxMaxY - mBBoxMinY;
	mAxes[ 2 ][ 2 ] = mBBoxMaxZ - mBBoxMinZ;

	// Assign a new position to each control point.
	for( int i = 0; i < mCtrlPtCounts[ 0 ]; i++ ) {
		for( int j = 0; j < mCtrlPtCounts[ 1 ]; j++ ) {
			for( int k = 0; k < mCtrlPtCounts[ 2 ]; k++ ) {
				double pt_x = mBBoxMinX + ( i / ( double )mSpanCounts[ 0 ] ) * mAxes[ 0 ][ 0 ];
				double pt_y = mBBoxMinY + ( j / ( double )mSpanCounts[ 1 ] ) * mAxes[ 1 ][ 1 ];
				double pt_z = mBBoxMinZ + ( k / ( double )mSpanCounts[ 2 ] ) * mAxes[ 2 ][ 2 ];
				setPositionTernary( i, j, k, pt_x, pt_y, pt_z );
			}
		}
	}
}

void addScaledVector( double point[ 3 ], const double point_to_add[ 3 ], double scale ) {
	point[ 0 ] += point_to_add[ 0 ] * scale;
	point[ 1 ] += point_to_add[ 1 ] * scale;
	point[ 2 ] += point_to_add[ 2 ] * scale;
}

// Evaluates the volume at (s, t, u) parameters
// where each parameter ranges from 0 to 1.
void evalTrivariate( double s, double t, double u, double eval_pt[ 3 ] ) {
	eval_pt[ 0 ] = eval_pt[ 1 ] = eval_pt[ 2 ] = 0.0;
	for( int i = 0; i < mCtrlPtCounts[ 0 ]; i++ ) {
		double point1[ 3 ] = { 0. };
		for( int j = 0; j < mCtrlPtCounts[ 1 ]; j++ ) {
			double point2[ 3 ] = { 0. };
			for( int k = 0; k < mCtrlPtCounts[ 2 ]; k++ ) {
				double *position = getPositionTernary( i, j, k );
				double poly_u = bernstein( mSpanCounts[ 2 ], k, u );
				addScaledVector( point2, position, poly_u );
			}
			double poly_t = bernstein( mSpanCounts[ 1 ], j, t );
			addScaledVector( point1, point2, poly_t );
		}
		double poly_s = bernstein( mSpanCounts[ 0 ], i, s );
		addScaledVector( eval_pt, point1, poly_s );
	}
}

void crossVectors( const double p1[ 3 ], const double p2[ 3 ], double result[ 3 ] ) {
	result[ 0 ] = p1[ 1 ] * p2[ 2 ] - p1[ 2 ] * p2[ 1 ];
	result[ 1 ] = p1[ 2 ] * p2[ 0 ] - p1[ 0 ] * p2[ 2 ];
	result[ 2 ] = p1[ 0 ] * p2[ 1 ] - p1[ 1 ] * p2[ 0 ];
}

double dot( const double p1[ 3 ], const double p2[ 3 ] ) {
	return p1[ 0 ] * p2[ 0 ] + p1[ 1 ] * p2[ 1 ] + p1[ 2 ] * p2[ 2 ];
}

// Converts the given point (x, y, z) in world space to (s, t, u) in parameter space.
void convertToParam( const double world_pt[ 3 ], double param[ 3 ] ) {
	// A vector from the minimum point of the bounding box to the given world point.
	double min2world[ 3 ] = {
		world_pt[ 0 ] - mBBoxMinX,
		world_pt[ 1 ] - mBBoxMinY,
		world_pt[ 2 ] - mBBoxMinZ };

	double cross[ 3 ][ 3 ];
	crossVectors( mAxes[ 1 ], mAxes[ 2 ], cross[ 0 ] );
	crossVectors( mAxes[ 0 ], mAxes[ 2 ], cross[ 1 ] );
	crossVectors( mAxes[ 0 ], mAxes[ 1 ], cross[ 2 ] );

	for( int i = 0; i < 3; i++ ) {
		double numer = dot( cross[ i ], min2world );
		double denom = dot( cross[ i ], mAxes[ i ] );
		param[ i ] = numer / denom;
	}
}

// Returns 1 if a and b are equal with 1e-6. 
bool equals( double a, double b )
{
	return ( a == b ) ||
		( ( a <= ( b + 1e-6 ) ) &&
		( a >= ( b - 1e-6 ) ) );
}

// Evaluates the volume at the given point in world space.
void evalWorld( const double world_pt[ 3 ], double eval_pt[ 3 ] ) {
	double param[ 3 ];
	convertToParam( world_pt, param );
	evalTrivariate( param[ 0 ], param[ 1 ], param[ 2 ], eval_pt );
}

WASM_EXPORT
void storeUndeformedVerts()
{
	for( int i = 0; i < mModelVertCount; i++ ) {
		mModelVertsUndeformedX[ i ] = mModelVertsX[ i ];
		mModelVertsUndeformedY[ i ] = mModelVertsY[ i ];
		mModelVertsUndeformedZ[ i ] = mModelVertsZ[ i ];
	}
}

WASM_EXPORT
double *dataModelVertsX() { return mModelVertsX; }
WASM_EXPORT
double *dataModelVertsY() { return mModelVertsY; }
WASM_EXPORT
double *dataModelVertsZ() { return mModelVertsZ; }

WASM_EXPORT
void deformModelVerts() {
	for( int i = 0; i < mModelVertCount; i++ ) {
		double world_pt[ 3 ] = { mModelVertsUndeformedX[ i ], mModelVertsUndeformedY[ i ], mModelVertsUndeformedZ[ i ] };
		double eval_pt[ 3 ];
		evalWorld( world_pt, eval_pt );

		if( equals( eval_pt[ 0 ], mModelVertsX[ i ] ) &&
			equals( eval_pt[ 1 ], mModelVertsY[ i ] ) &&
			equals( eval_pt[ 2 ], mModelVertsZ[ i ] ) )
			continue;

		mModelVertsX[ i ] = eval_pt[ 0 ];
		mModelVertsY[ i ] = eval_pt[ 1 ];
		mModelVertsZ[ i ] = eval_pt[ 2 ];
	}
}

WASM_EXPORT
double *dataVolumePointsX() { return mVolumePointsX; }
WASM_EXPORT
double *dataVolumePointsY() { return mVolumePointsY; }
WASM_EXPORT
double *dataVolumePointsZ() { return mVolumePointsZ; }

WASM_EXPORT
void evalVolumePoints() {
	double multipliers[ 3 ] = {
		1 / ( double )mVolumeSpanCount[ 0 ],
		1 / ( double )mVolumeSpanCount[ 1 ],
		1 / ( double )mVolumeSpanCount[ 2 ] };

	int index_vol_pt = 0;
	for( int i = 0; i < mVolumePointCount[ 0 ]; i++ ) {
		double s = i * multipliers[ 0 ];
		for( int j = 0; j <  mVolumePointCount[ 1 ]; j++ ) {
			double t = j * multipliers[ 1 ];
			for( int k = 0; k <  mVolumePointCount[ 2 ]; k++ ) {
				double u = k * multipliers[ 2 ];
				double eval_pt[ 3 ];
				evalTrivariate( s, t, u, eval_pt );

				bool is_equal =
					equals( eval_pt[ 0 ], mVolumePointsX[ index_vol_pt ] ) &&
					equals( eval_pt[ 1 ], mVolumePointsY[ index_vol_pt ] ) &&
					equals( eval_pt[ 2 ], mVolumePointsZ[ index_vol_pt ] );

				index_vol_pt++;
				if( is_equal )
					continue;

				mVolumePointsX[ index_vol_pt ] = eval_pt[ 0 ];
				mVolumePointsY[ index_vol_pt ] = eval_pt[ 1 ];
				mVolumePointsZ[ index_vol_pt ] = eval_pt[ 2 ];
			}
		}
	}
}
