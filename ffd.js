// Author: Glenn Chun
// Date: April 15, 2016
// As my first coding exercise in JavaScript, I implemented the FFD algorithm presented
// in Sederberg & Parry's '86 paper, Free-Form Deformation of Solid Geometric Models.

function FFD() {
	///////////////////////////////////////////////////////////////////////////
	// Privileged members
	///////////////////////////////////////////////////////////////////////////

	// Returns the bounding box of the undeformed lattice.
	this.getBoundingBox = function() { return mBBox; }

	// Returns the number of control points on the given parameter direction.
	// direction: 0 for S, 1 for T, and 2 for U.
	this.getCtrlPtCount = function(direction) { return mCtrlPtCounts[direction]; }

	// Returns the total number of control points.    
	this.getTotalCtrlPtCount = function() { return mTotalCtrlPtCount; }

	// Converts the given ternary index to a unary index.
	this.getIndex = function(i, j, k) {
		return i * mCtrlPtCounts[1] * mCtrlPtCounts[2] + j * mCtrlPtCounts[2] + k;
	};

	// Rebuilds the lattice with new control points.
	this.rebuildLattice = function(bbox, span_counts) {
		// Do not rebuild the lattice if the bounding box and span counts are the same as before.
		if (mBBox.equals(bbox) &&
            mSpanCounts[0] == span_counts[0] &&
            mSpanCounts[1] == span_counts[1] &&
            mSpanCounts[2] == span_counts[2])
			return;

		mBBox = bbox;
		mSpanCounts = span_counts;
		mCtrlPtCounts = [mSpanCounts[0] + 1, mSpanCounts[1] + 1, mSpanCounts[2] + 1];
		mTotalCtrlPtCount = mCtrlPtCounts[0] * mCtrlPtCounts[1] * mCtrlPtCounts[2];

		// Set the S/T/U axes.
		mAxes[0].x = mBBox.max.x - mBBox.min.x;
		mAxes[1].y = mBBox.max.y - mBBox.min.y;
		mAxes[2].z = mBBox.max.z - mBBox.min.z;

		// Reset the array for control points.
		mCtrlPts = new Array(mTotalCtrlPtCount);

		// Assign a new position to each control point. 
		for (var i = 0; i < mCtrlPtCounts[0]; i++) {
			for (var j = 0; j < mCtrlPtCounts[1]; j++) {
				for (var k = 0; k < mCtrlPtCounts[2]; k++) {
					var position = new THREE.Vector3(
                        mBBox.min.x + (i / mSpanCounts[0]) * mAxes[0].x,
                        mBBox.min.y + (j / mSpanCounts[1]) * mAxes[1].y,
                        mBBox.min.z + (k / mSpanCounts[2]) * mAxes[2].z
                    );
					this.setPositionTernary(i, j, k, position);
				}
			}
		}
	};

	// Evaluates the volume at (s, t, u) parameters
	// where each parameter ranges from 0 to 1.
	this.evalTrivariate = function(s, t, u) {
		var eval_pt = new THREE.Vector3(0, 0, 0);
		for (var i = 0; i < mCtrlPtCounts[0]; i++) {
			var point1 = new THREE.Vector3(0, 0, 0);
			for (var j = 0; j < mCtrlPtCounts[1]; j++) {
				var point2 = new THREE.Vector3(0, 0, 0);
				for (var k = 0; k < mCtrlPtCounts[2]; k++) {
					var position = this.getPositionTernary(i, j, k);
					var poly_u = bernstein(mSpanCounts[2], k, u);
					point2.addScaledVector(position, poly_u);
				}
				var poly_t = bernstein(mSpanCounts[1], j, t);
				point1.addScaledVector(point2, poly_t);
			}
			var poly_s = bernstein(mSpanCounts[0], i, s);
			eval_pt.addScaledVector(point1, poly_s);
		}
		return eval_pt;
	};

	// Converts the given point (x, y, z) in world space to (s, t, u) in parameter space.
	this.convertToParam = function(world_pt) {
		// A vector from the mininmum point of the bounding box to the given world point.
		var min2world = new THREE.Vector3(world_pt.x, world_pt.y, world_pt.z);
		min2world.sub(mBBox.min);

		var cross = [new THREE.Vector3(), new THREE.Vector3(), new THREE.Vector3()];
		cross[0].crossVectors(mAxes[1], mAxes[2]);
		cross[1].crossVectors(mAxes[0], mAxes[2]);
		cross[2].crossVectors(mAxes[0], mAxes[1]);

		var param = new THREE.Vector3();
		for (var i = 0; i < 3; i++) {
			var numer = cross[i].dot(min2world);
			var denom = cross[i].dot(mAxes[i]);
			param.setComponent(i, numer / denom);
		}
		return param;
	};

	// Returns the position of the control point at the given unary index.
	this.getPosition = function(index) {
		return mCtrlPts[index];
	};

	// Sets the position of the control point at the given unary index.
	this.setPosition = function(index, position) {
		mCtrlPts[index] = position;
	};

	// Returns the position of the control point at the given ternary index.
	this.getPositionTernary = function(i, j, k) {
		return mCtrlPts[this.getIndex(i, j, k)];
	};

	// Sets the position of the control point at the given ternary index.
	this.setPositionTernary = function(i, j, k, position) {
		mCtrlPts[this.getIndex(i, j, k)] = position;
	};

	///////////////////////////////////////////////////////////////////////////
	// Private members
	///////////////////////////////////////////////////////////////////////////

	// The bounding box of the undeformed lattice.
	var mBBox = new THREE.Box3();

	// Number of spans in each parameter direction, S/T/U.
	var mSpanCounts = [0, 0, 0];

	// Number of control points in each parameter direction, S/T/U.
	var mCtrlPtCounts = [0, 0, 0];

	// Total number of control points.
	var mTotalCtrlPtCount = 0;

	// S/T/U axes.
	var mAxes = [new THREE.Vector3(0, 0, 0), new THREE.Vector3(0, 0, 0), new THREE.Vector3(0, 0, 0)];

	// Positions of all control points.
	var mCtrlPts = [];

	// Returns n-factorial.
	function facto(n) {
		var fac = 1;
		for (var i = n; i > 0; i--)
			fac *= i;
		return fac;
	};

	// Returns the Bernstein polynomial in one parameter, u.
	function bernstein(n, k, u) {
		// Binomial coefficient
		var coeff = facto(n) / (facto(k) * facto(n - k));
		return coeff * Math.pow(1 - u, n - k) * Math.pow(u, k);
	};
}

///////////////////////////////////////////////////////////////////////////
// Public members
///////////////////////////////////////////////////////////////////////////

// Evaluates the volume at the given point in world space.
FFD.prototype.evalWorld = function(world_pt) {
	var param = this.convertToParam(world_pt);
	return this.evalTrivariate(param.x, param.y, param.z);
}
