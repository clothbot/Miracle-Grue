/**
   MiracleGrue - Model Generator for toolpathing. <http://www.grue.makerbot.com>
   Copyright (C) 2011 Far McKon <Far@makerbot.com>, Hugo Boyer (hugo@makerbot.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

*/

#include <iomanip>
#include <set>
#include <fstream>


#include <stdint.h>
#include <cstring>

#include "meshy.h"
#include "shrinky.h"
#include "scadtubefile.h"



using namespace mgl;
using namespace std;


bool mgl::tequals(Scalar a, Scalar b, Scalar tol)
{
	return SCALAR_ABS(a-b) < tol;
}

//
//// returns the angle between 3 points
//Scalar mgl::angleFromPoint2s(const Vector2 &i, const Vector2 &j, const Vector2 &k)
//{
//	Vector2 a = i - j;
//	Vector2 b = j - k;
//	Scalar theta = angleFromVector2s(a,b);
//	return theta;
//}


std::ostream& mgl::operator<<(ostream& os, const mgl::Vector3& v)
{
	os << "[" << v[0] << ", " << v[1] << ", " << v[2] << "]";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Polygon& polygon)
{
	for(unsigned int i=0; i< polygon.size(); i++)
	{
		os << i << ") " << polygon[i] << endl;
	}
	return os;
}

//Vector2 mgl::rotate2d(const Vector2 &p, Scalar angle)
//{
//	// rotate point
//	Scalar s = SCALAR_SIN(angle); // radians
//	Scalar c = SCALAR_COS(angle);
//	Vector2 rotated;
//	rotated.x = p.x * c - p.y * s;
//	rotated.y = p.x * s + p.y * c;
//	return rotated;
//}

void mgl::rotatePolygon(Polygon& polygon, Scalar angle)
{
	for(unsigned int i=0; i<polygon.size(); i++)
	{
		const Vector2 &p = polygon[i];
		polygon[i] = p.rotate2d(angle);
	}
}

void mgl::rotatePolygons(Polygons& polygons, Scalar angle)
{
	for(unsigned int i=0; i<polygons.size(); i++)
	{
		Polygon& polygon = polygons[i];
		rotatePolygon(polygon, angle);
	}
}


void dumpSegments(const char* prefix, const std::vector<LineSegment2> &segments)
{
	cout << prefix << "segments = [ // " << segments.size() << " segments" << endl;
    for(size_t id = 0; id < segments.size(); id++)
    {
    	LineSegment2 seg = segments[id];
    	cout << prefix << " [[" << seg.a << ", " << seg.b << "]], // " << id << endl;
    }
    cout << prefix << "]" << endl;
    cout << "// color([1,0,0.4,1])loop_segments(segments,0.050000);" << endl;
}

void dumpInsets(const std::vector<SegmentTable> &insetsForLoops)
{
	for (unsigned int i=0; i < insetsForLoops.size(); i++)
	{
		const SegmentTable &insetTable =  insetsForLoops[i];
		cout << "Loop " << i << ") " << insetTable.size() << " insets"<<endl;

		for (unsigned int i=0; i <insetTable.size(); i++)
		{
			const std::vector<LineSegment2 >  &loop = insetTable[i];
			cout << "   inset " << i << ") " << loop.size() << " segments" << endl;
			dumpSegments("        ",loop);
		}
	}
}



std::ostream& mgl::operator<<(ostream& os, const Limits& l)
{
	os << "[" << l.xMin << ", " << l.yMin << ", " << l.zMin << "] [" << l.xMax << ", " << l.yMax << ", "<< l.zMax  << "]";
	return os;
}

ostream& mgl::operator <<(ostream &os,const Vector2 &pt)
{
    os << "[" << pt.x << ", " << pt.y << "]";
    return os;
}


#ifdef __BYTE_ORDER
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define I_AM_LITTLE_ENDIAN
# else
#  if __BYTE_ORDER == __BIG_ENDIAN
#   define I_AM_BIG_ENDIAN
#  else
#error "Unknown byte order!"
#  endif
# endif
#endif /* __BYTE_ORDER */

#ifdef I_AM_BIG_ENDIAN
static inline void convertFromLittleEndian32(uint8_t* bytes)
{
    uint8_t tmp = bytes[0];
    bytes[0] = bytes[3];
    bytes[3] = tmp;
    tmp = bytes[1];
    bytes[1] = bytes[2];
    bytes[2] = tmp;
}
static inline void convertFromLittleEndian16(uint8_t* bytes)
{
    uint8_t tmp = bytes[0];
    bytes[0] = bytes[1];
    bytes[1] = tmp;
}
#else
static inline void convertFromLittleEndian32(uint8_t* bytes)
{
}
static inline void convertFromLittleEndian16(uint8_t* bytes)
{
}
#endif


size_t mgl::writeMeshyToStl(mgl::Meshy &meshy, const char* filename)
{
	meshy.writeStlFile(filename);
}


size_t mgl::loadMeshyFromStl(mgl::Meshy &meshy, const char* filename)
{

	struct vertexes_t {
		double nx, ny, nz;
		double x1, y1, z1;
		double x2, y2, z2;
		double x3, y3, z3;
		uint16_t attrBytes;
	};

	union {
		struct vertexes_t vertexes;
		uint8_t bytes[sizeof(vertexes_t)];
	} tridata;

	union
	{
		uint32_t intval;
		uint16_t shortval;
		uint8_t bytes[4];
	} intdata;

	size_t facecount = 0;

	uint8_t buf[512];
	FILE *f = fopen(filename, "rb");
	if (!f)
	{
		string msg = "Can't open \"";
		msg += filename;
		msg += "\". Check that the file name is correct and that you have sufficient privileges to open it.";
		MeshyException problem(msg.c_str());
		throw (problem);
	}

	if (fread(buf, 1, 5, f) < 5) {
		string msg = "\"";
		msg += filename;
		msg += "\" is empty!";
		MeshyException problem(msg.c_str());
		throw (problem);
	}
	bool isBinary = true;
	if (!strncasecmp((const char*) buf, "solid", 5)) {
		isBinary = false;
	}
	if (isBinary) {
		// Binary STL file
		// Skip remainder of 80 character comment field
		if (fread(buf, 1, 75, f) < 75) {
			string msg = "\"";
			msg += filename;
			msg += "\" is not a valid stl file";
			MeshyException problem(msg.c_str());
			throw (problem);
		}
		// Read in triangle count
		if (fread(intdata.bytes, 1, 4, f) < 4) {
			string msg = "\"";
			msg += filename;
			msg += "\" is not a valid stl file";
			MeshyException problem(msg.c_str());
			throw (problem);
		}
		convertFromLittleEndian32(intdata.bytes);
		uint32_t tricount = intdata.intval;
		while (!feof(f) && tricount-- > 0) {
			if (fread(tridata.bytes, 1, 3 * 4 * 4 + 2, f) < 3 * 4 * 4 + 2) {
				break;
			}
			for (int i = 0; i < 3 * 4; i++) {
				convertFromLittleEndian32(tridata.bytes + i * 4);
			}
			convertFromLittleEndian16((uint8_t*) &tridata.vertexes.attrBytes);

			vertexes_t &v = tridata.vertexes;
			Vector3 pt1(v.x1, v.y1, v.z1);
			Vector3 pt2(v.x2, v.y2, v.z2);
			Vector3 pt3(v.x3, v.y3, v.z3);

			Triangle3 triangle(pt1, pt2, pt3);
			meshy.addTriangle(triangle);

			facecount++;
		}
		fclose(f);
	} else {
		// ASCII STL file
		// Gobble remainder of solid name line.
		fgets((char*) buf, sizeof(buf), f);
		while (!feof(f)) {
			fscanf(f, "%80s", buf);
			if (!strcasecmp((char*) buf, "endsolid")) {
				break;
			}
			vertexes_t &v = tridata.vertexes;
			bool success = true;
			if (fscanf(f, "%*s %lf %lf %lf", &v.nx, &v.ny, &v.nz) < 3)
				success = false;
			if (fscanf(f, "%*s %*s") < 0)
				success = false;
			if (fscanf(f, "%*s %lf %lf %lf", &v.x1, &v.y1, &v.z1) < 3)
				success = false;
			if (fscanf(f, "%*s %lf %lf %lf", &v.x2, &v.y2, &v.z2) < 3)
				success = false;
			if (fscanf(f, "%*s %lf %lf %lf", &v.x3, &v.y3, &v.z3) < 3)
				success = false;
			if (fscanf(f, "%*s")< 0)
				success = false;
			if (fscanf(f, "%*s")< 0)
				success = false;
			if(!success)
			{
				stringstream msg;
				msg << "Error reading face " << facecount << " in file \"" << filename << "\"";
				MeshyException problem(msg.str().c_str());
				cout << msg.str().c_str()<< endl;
				cout << buf << endl;
				throw(problem);
			}
			Triangle3 triangle(Vector3(v.x1, v.y1, v.z1),	Vector3(v.x2, v.y2, v.z2),	Vector3(v.x3, v.y3, v.z3));
			meshy.addTriangle(triangle);

			facecount++;
		}
		fclose(f);
	}
	return facecount;
}


/**
 * Assuming the 2d points are on a plane, and that point order indicates a
 * vector out of that plane, returns magnitude of that vector.
 *  See Also: Right-hand-rule
 * Ex: ((0,0)(0,1)(1,0))  returns -1 (normal points negative Z out of plane)
 * Ex: ((1,0)(0,0)(0,1))  returns 1 (normal points positive Z out of plane)
 */
Scalar mgl::AreaSign(const Vector2 &a, const Vector2 &b, const Vector2 &c)
{
	Scalar area2;
    area2 = (b[0] - a[0] ) * (Scalar)( c[1] - a[1]) -
            (c[0] - a[0] ) * (Scalar)( b[1] - a[1]);

    return area2;
}


/**
 * @returns true if the triangle of these vectors has a negative index,
 * false otherwise
 */
bool mgl::convexVertex(const Vector2 &i, const Vector2 &j, const Vector2 &k)
{
	return AreaSign(i,j,k) < 0;
}

std::ostream& mgl::operator << (std::ostream &os, const LineSegment2 &s)
{
	os << "[ " << s.a << ", " << s.b << "]";
	return os;
}

/**
 * @returns true if the passed line segments are colinear within the tolerance tol
 */
bool mgl::collinear(const LineSegment2 &prev, const LineSegment2 &current, Scalar tol, Vector2 &mid)
{

	Scalar x1 = prev.a[0];
	Scalar y1 = prev.a[1];
	mid.x = 0.5 * (prev.b[0] + current.a[0]);
	mid.y = 0.5 * (prev.b[1] + current.a[1]);
	Scalar x3 = current.b[0];
	Scalar y3 = current.b[1];

	Scalar c = ((mid.x - x1) * (y3 - y1) - (x3 - x1) * (mid.y - y1));
	bool r = tequals(c, 0, tol);
	return r;
}

/**
 * @returns a new LineSegment2, elongated to be normalized to a unit vector
 */
LineSegment2 mgl::elongate(const LineSegment2 &s, Scalar dist)
{
	LineSegment2 segment(s);
	Vector2 l = segment.b - segment.a;
	l.normalise();
	l *= dist;
	segment.b += l;
	return segment;
}

/**
 * @returns a new line segment. Of what, I don't know. Wasn't documented.
 */
LineSegment2 mgl::prelongate(const LineSegment2 &s, Scalar dist)
{
	LineSegment2 segment(s);
	Vector2 l = segment.a - segment.b;
	l.normalise();
	l *= dist;
	segment.a += l;
	return segment;
}
