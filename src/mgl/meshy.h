/**
   MiracleGrue - Model Generator for toolpathing. <http://www.grue.makerbot.com>
   Copyright (C) 2011 Far McKon <Far@makerbot.com>, Hugo Boyer (hugo@makerbot.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

*/


#ifndef MESHY_H_
#define MESHY_H_

#include <iomanip>
#include <set>
#include <fstream>

#include "segment.h"
#include "limits.h"
#include "abstractable.h"


#ifdef OMPFF
#include <omp.h>
#endif

///
/// Meshy-ness: that property that links all these triangles together
///

namespace mgl // serious about triangles
{




//
// Exception class for meshy problems
//
class MeshyException : public Exception
{
public:
	MeshyException(const char *msg)
	 :Exception(msg)
	{

	}

};

// simple class that writes
// a simple text file STL
class StlWriter
{

//solid Default
//  facet normal 1.435159e-01 2.351864e-02 9.893685e-01
//    outer loop
//      vertex -7.388980e-02 -2.377973e+01 6.062650e+01
//      vertex -1.193778e-01 -2.400027e+01 6.063834e+01
//      vertex -4.402440e-06 -2.490700e+01 6.064258e+01
//    endloop
//  endfacet
//endsolid Default

	std::ofstream out;
	std::string solidName;

public:
	void open(const char* fileName, const char *solid="Default")

	{
		solidName = solid;
		out.open(fileName);
		if(!out)
		{
			std::stringstream ss;
			ss << "Can't open \"" << fileName << "\"";
			MeshyException problem(ss.str().c_str());
			throw (problem);
		}

		// bingo!
		out << std::scientific;
		out << "solid " << solidName << std::endl;

	}

	void writeTriangle(const Triangle3& t)
	{
		// normalize( (v1-v0) cross (v2 - v0) )
		// y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x

		Vector3 n = t.normal();
		out << " facet normal " << n[0] << " " << n[1] << " " << n[2] << std::endl;
		out << "  outer loop"<< std::endl;
		out << "    vertex " << t[0].x << " " << t[0].y << " " << t[0].z << std::endl;
		out << "    vertex " << t[1].x << " " << t[1].y << " " << t[1].z << std::endl;
		out << "    vertex " << t[2].x << " " << t[2].y << " " << t[2].z << std::endl;
		out << "  endloop" << std::endl;
		out << " endfacet" << std::endl;
	}

	void close()
	{
		out << "endsolid " << solidName << std::endl;
		out.close();
	}

};


/**
 *
 * A Mesh class
 */
class Meshy
{

	mgl::Limits limits; 	/// Bounding box for the model
	std::vector<Triangle3>  allTriangles; /// every triangle in the model.
	/// for each slice, a list of indicies, each index is a lookup into vector
	// allTriangles
	SliceTable sliceTable;

	// Ze tape measure, for Z
	LayerMeasure zTapeMeasure;

public:


	/// requires firstLayerSlice height, and general layer height
	Meshy(Scalar firstSliceZ, Scalar layerH)
		:zTapeMeasure(firstSliceZ, layerH)
	{ 	}

	const std::vector<Triangle3> &readAllTriangles() const
	{
		return allTriangles;
	}

	const Limits& readLimits() const
	{
		return limits;
	}

	const LayerMeasure& readLayerMeasure() const
	{
		return zTapeMeasure;
	}

	const SliceTable &readSliceTable() const
	{
		return sliceTable;
	}


	//
	// Adds a triangle to the global array and for each slice of interest
	//
	void addTriangle(Triangle3 &t)
	{

		Vector3 a, b, c;
		t.zSort(a,b,c);

		unsigned int minSliceIndex = this->zTapeMeasure.zToLayerAbove(a.z);
		if(minSliceIndex > 0)
			minSliceIndex --;

		unsigned int maxSliceIndex = this->zTapeMeasure.zToLayerAbove(c.z);
		if (maxSliceIndex - minSliceIndex > 1)
			maxSliceIndex --;

//		std::cout << "Min max index = [" <<  minSliceIndex << ", "<< maxSliceIndex << "]"<< std::endl;
//		std::cout << "Max index =" <<  maxSliceIndex << std::endl;
		unsigned int currentSliceCount = sliceTable.size();
		if (maxSliceIndex >= currentSliceCount)
		{
			unsigned int newSize = maxSliceIndex+1;
			sliceTable.resize(newSize); // make room for potentially new slices
//			std::cout << "- new slice count: " << sliceTable.size() << std::endl;
		}

		allTriangles.push_back(t);

		size_t newTriangleId = allTriangles.size() -1;

//		 std::cout << "adding triangle " << newTriangleId << " to layer " << minSliceIndex  << " to " << maxSliceIndex << std::endl;
		for (size_t i= minSliceIndex; i<= maxSliceIndex; i++)
		{
			TriangleIndices &trianglesForSlice = sliceTable[i];
			trianglesForSlice.push_back(newTriangleId);
//			std::cout << "   !adding triangle " << newTriangleId << " to layer " << i  << " (size = " << trianglesForSlice.size() << ")" << std::endl;
		}

		limits.grow(t[0]);
		limits.grow(t[1]);
		limits.grow(t[2]);


	}


	void dump(std::ostream &out)
	{
		out << "dumping " << this << std::endl;
		out << "Nb of triangles: " << allTriangles.size() << std::endl;
		size_t sliceCount = sliceTable.size();

		out << "triangles per slice: (" << sliceCount << " slices)" << std::endl;
		for (size_t i= 0; i< sliceCount; i++)
		{
			TriangleIndices &trianglesForSlice = sliceTable[i];
			//trianglesForSlice.push_back(newTriangleId);
			out << "  slice " << i << " size: " << trianglesForSlice.size() << std::endl;
			//cout << "adding triangle " << newTriangleId << " to layer " << i << std::endl;
		}
	}


public:

	size_t triangleCount() {
		return allTriangles.size();
		std::cout << "all triangle count" << allTriangles.size();
	}

	void writeStlFile(const char* fileName) const
	{
		StlWriter out;
		out.open(fileName);
		size_t triCount = allTriangles.size();
		for (size_t i= 0; i < triCount; i++)
		{
			const Triangle3 &t = allTriangles[i];
			out.writeTriangle(t);
		}
		out.close();
		// cout << fileName << " written!"<< std::endl;

	}

	void writeStlFileForLayer(unsigned int layerIndex, const char* fileName) const
	{

		StlWriter out;
		out.open(fileName);

		const TriangleIndices &trianglesForSlice = sliceTable[layerIndex];
		for(std::vector<index_t>::const_iterator i = trianglesForSlice.begin(); i!= trianglesForSlice.end(); i++)
		{
			index_t index = *i;
			const Triangle3 &t = allTriangles[index];
			out.writeTriangle(t);
		}
		out.close();
		// cout << fileName << " written!"<< std::endl;
	}

};


void writeMeshyToStl(mgl::Meshy &meshy, const char* filename);

size_t loadMeshyFromStl(mgl::Meshy &meshy, const char* filename);



// compile time enabled
// Multi threaded stuff
//
#ifdef OMPFF
// a lock class for multithreaded sync
class OmpGuard {
public:
    //Acquire the lock and store a pointer to it
	OmpGuard (omp_lock_t &lock)
	:lock_ (&lock)
	{
		acquire();
	}
    void acquire ()
    {
    	omp_set_lock (lock_);
    }

    void release ()
    {
    	omp_unset_lock (lock_);
    }

    ~OmpGuard ()
    {
    	release();
    }

private:
    omp_lock_t *lock_;  // pointer to our lock

};
#endif




} // namespace



#endif
