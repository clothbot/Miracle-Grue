/**
   MiracleGrue - Model Generator for toolpathing. <http://www.grue.makerbot.com>
   Copyright (C) 2011 Far McKon <Far@makerbot.com>, Hugo Boyer (hugo@makerbot.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

*/


#ifndef SCAD_TUBEFILE_H_
#define SCAD_TUBEFILE_H_

#include <iostream>
#include <ostream>
#include <fstream>
#include <sstream>

//#include "meshy.h"
#include "abstractable.h"
#include "limits.h"
#include "mgl.h"


namespace mgl
{

class ScadException : public Exception
{
public:
	ScadException(const char *msg)
	 :Exception(msg)
	{

	}

};


//
// OpenSCAD file generator http://www.openscad.org/
//
class ScadTubeFile
{
	std::ofstream out;
//	double layerH;
//	double layerW;

	std::string filename;

public:
	ScadTubeFile()
	:filename("")
	{}

	void open(const char* path)
	{
		assert(isOpened() == false);
		filename = path;
		out.open(filename.c_str(), std::ios::out);
		if(!out.good())
		{
			std::stringstream ss;
			ss << "Can't open \"" << filename.c_str() << "\"";
			std::cout << "ERROR: " << ss.str() << std::endl;
			ScadException problem(ss.str().c_str());
			throw (problem);
		}

		out.precision(5);
	    out.setf(std::ios::fixed);


	}

	std::string getScadFileName() const
	{
		return filename;
	}

	bool isOpened()
	{
		unsigned int l = filename.length();
		return (l > 0);
	}



	std::ostream &getOut(){return out;}

	void writeHeader()
    {
        out << "module corner(x, y, z, diameter, faces, thickness_over_width ){" << std::endl;
        out << "	translate([x, y, z])  scale([1,1,thickness_over_width]) sphere( r = diameter/2, $fn = faces );" << std::endl;
        out << "}" << std::endl;
        out << std::endl;
        out << "module tube(x1, y1, z1, x2, y2, z2, diameter1, diameter2, faces, thickness_over_width)" << std::endl;
        out << "{" << std::endl;
        out << "	length = sqrt( pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2) );" << std::endl;
        out << "	alpha = ( (y2 - y1 != 0) ? atan( (z2 - z1) / (y2 - y1) ) : 0 );" << std::endl;
        out << " 	beta = 90 - ( (x2 - x1 != 0) ? atan( (z2 - z1) / (x2 - x1) ) :  0 );" << std::endl;
        out << "	gamma =  ( (x2 - x1 != 0) ? atan( (y2 - y1) / (x2 - x1) ) : ( (y2 - y1 >= 0) ? 90 : -90 ) ) + ( (x2 - x1 >= 0) ? 0 : -180 );" << std::endl;
        out << "	// echo(Length = length, Alpha = alpha, Beta = beta, Gamma = gamma);	" << std::endl;
        out << "	translate([x1, y1, z1])" << std::endl;
        out << "	rotate([ 0, beta, gamma])" << std::endl;
        out << "		scale([thickness_over_width,1,1])" << std::endl;
        out << "			rotate([0,0,90]) cylinder(h = length, r1 = diameter1/2, r2 = diameter2/2, center = false, $fn = faces );" << std::endl;
        out << "}" << std::endl;
    }

	void close()
	{
		out.close();
		filename = "";
	}



	void writeOutlines(const Polygons &loops, Scalar z, int slice)
	{

		out << "module outlines_" << slice << "()" << std::endl;
		out << "{" << std::endl;
		out << "    points =[" << std::endl;
		for (size_t i=0; i < loops.size(); i++)
		{
			out << "               [" << std::endl;
			const Polygon& loop  = loops[i];
			for (size_t j=0; j < loop.size(); j++)
			{
				Scalar x = loop[j].x;
				Scalar y = loop[j].y;
				out << "                   [" << x << ", " <<  y << ", " << z << "]," << std::endl;
			}
			out << "                ]," << std::endl;

		}
		out << "              ];" << std::endl;

		out << "    segments =[" << std::endl;
		for (size_t i=0; i < loops.size(); i++)
		{
			out << "               [" << std::endl;
			const Polygon& loop  = loops[i];
			for (size_t j=0; j < loop.size()-1; j++)
			{
				int a = j;
				int b = j+1;
				out << "                  [" << a << ", " <<  b << "]," << std::endl;
			}
			out << "              ]," << std::endl;
		}
		out << "            ];" << std::endl;

		out << std::endl;
		for (size_t i=0; i < loops.size(); i++)
		{
			out << "    outline(points[" << i << "], segments[" << i << "] );" << std::endl;
		}
		out << std::endl;
		out << "}" << std::endl;

	}

	void writePolygons(const char* moduleName,
						const char* implementation,
							const Polygons &polygons,
								Scalar z,
									int slice)
	{

		//std::cout << "<writePolygons: " << polygons.size() << " polygons >"<< std::endl;

		out << "module " << moduleName << slice << "()" << std::endl;
		out << "{" << std::endl;
		out << "    points =[" << std::endl;

		for (size_t i=0; i < polygons.size(); i++)
		{
			out << "               [" << std::endl;
			const Polygon& poly  = polygons[i];

			//std::cout << "   Polygon " << i << ": " << poly.size() << " points "<< std::endl;

			for (size_t j=0; j < poly.size(); j++)
			{
				Scalar x = poly[j].x;
				Scalar y = poly[j].y;
				out << "                   [" << x << ", " <<  y << ", " << z << "]," << std::endl;
			}
			out << "                ]," << std::endl;

		}
		out << "              ];" << std::endl;


		out << "    segments =[" << std::endl;

		size_t polysCount = polygons.size();
		for (size_t i=0; i < polysCount; i++)
		{

			out << "               [" << std::endl;
			const Polygon& poly  = polygons[i];
			size_t polyCount =  poly.size();

			for (size_t j=0; j < polyCount-1; j++)
			{   // std::cout << "  writePolygons: " << j << " polycount: " << polyCount << std::endl;
				int a = j;
				int b = j+1;
				out << "                  [" << a << ", " <<  b << "]," << std::endl;
			}
			out << "              ]," << std::endl;
		}
		out << "            ];" << std::endl;



		out << std::endl;
		for (size_t i=0; i < polygons.size(); i++)
		{
			out << "    " <<  implementation << "(points[" << i << "], segments[" << i << "] );" << std::endl;


		}
		out << std::endl;
		out << "}" << std::endl;

		// std::cout << "</writePolygons>" << std::endl;
	}

	static Scalar segment3(	std::ostream &out,
							const char* indent,
							const char* variableName,
							const std::vector<LineSegment2> &segments,
							Scalar z,
							Scalar dz)
	{
		out << indent << variableName<< " =[" << std::endl;
		for (size_t i=0; i < segments.size(); i++)
		{
			const LineSegment2& segment  = segments[i];
			Scalar ax = segment.a[0];
			Scalar ay = segment.a[1];
			Scalar az = z;

			Scalar bx = segment.b[0];
			Scalar by = segment.b[1];
			Scalar bz = z;

			out << indent << indent;
			out << "[[" << ax << ", " <<  ay << ", " << az << "],";
			out << "[ " << bx << ", " <<  by << ", " << bz << "]],";
			out << std::endl;
			z += dz;
		}
		out << indent << indent <<  "];" << std::endl;
		return z;
	}

	Scalar writeSegments3(	const char* name,
							const char* implementation,
							const std::vector<LineSegment2> &segments,
							Scalar z,
							Scalar dz,
							int slice)
	{
		out << "module " << name << slice << "()" << std::endl;
		out << "{" << std::endl;
		z = segment3(out, "    ", "segments", segments, z, dz);
		out << "    " <<  implementation << "(segments);" << std::endl;
		out << "}" << std::endl;
		out << std::endl;
		return z;
	}

	void writeSegments2(	const char* name,
							const char* implementation,
							const std::vector<LineSegment2> &segments,
							Scalar z,
							int slice)
	{
		out << "module " << name << slice << "()" << std::endl;
		out << "{" << std::endl;
		out << "    segments =[" << std::endl;
		for (size_t i=0; i < segments.size(); i++)
		{
			const LineSegment2& segment  = segments[i];
			Scalar ax = segment.a[0];
			Scalar ay = segment.a[1];
			Scalar bx = segment.b[0];
			Scalar by = segment.b[1];

			out << "                   ";
			out << "[[" << ax << ", " <<  ay <<  "],";
			out << "[ " << bx << ", " <<  by <<  "]],";
			out << std::endl;
		}

		out << "              ];" << std::endl;
		out << "    " <<  implementation << "(segments," << z <<");" << std::endl;
		out << "}" << std::endl;
		out << std::endl;
	}

	// writes a list of triangles into a polyhedron.
	// It is used to display the triangles involved in a slice (layerIndex).

	void writeTrianglesModule(	const char* name,
								const std::vector<Triangle3>  &allTriangles,
								const TriangleIndices &trianglesForSlice,
								unsigned int layerIndex)
	{
		std::stringstream ss;
		ss.setf(std::ios::fixed);

		ss << "module " << name << layerIndex << "(col=[1,0,0,1])" << std::endl;
		ss << "{" << std::endl;
		ss << "    color(col) polyhedron ( points = [";

		ss << std::dec; // set decimal format for floating point numbers

		for(size_t i=0; i< trianglesForSlice.size(); i++ )
		{

			index_t index = trianglesForSlice[i];
			const Triangle3 &t = allTriangles[index];
			ss << "    [" << t[0].x << ", " << t[0].y << ", " << t[0].z << "], ";
			ss <<     "[" << t[1].x << ", " << t[1].y << ", " << t[1].z << "], ";
			ss <<     "[" << t[2].x << ", " << t[2].y << ", " << t[2].z << "], // tri " << i << std::endl;
		}

		ss << "]," << std::endl;
		ss << "triangles = [" ;

		for (size_t i=0; i < trianglesForSlice.size(); i++)
		{
			int tri = i * 3;
			ss << "    [" << tri << ", " << tri+1 << ", " << tri + 2 << "], " << std::endl;
		}

		ss << "]);" << std::endl;
		ss << "}" << std::endl;
		ss << std::endl;
		out << ss.str();
	}
/*
	void writeTrianglesModule(const char* name, const Meshy &mesh,
								unsigned int layerIndex)
	{
		const TriangleIndices &trianglesForSlice = mesh.readSliceTable()[layerIndex];
		const std::vector<Triangle3>  &allTriangles = mesh.readAllTriangles();
		writeTrianglesModule(name, allTriangles, trianglesForSlice, layerIndex);
	}
*/
private:

public:

	void writeMinMax(const char*name, const char* implementation, int count)
	{
		out << "module "<< name << "(min=0, max=" << count-1 <<")" << std::endl;
		out << "{" << std::endl;
		for(int i=0; i< count; i++)
		{
			out << "	if(min <= "<< i <<" && max >=" << i << ")" << std::endl;
			out << "	{" << std::endl;
			out << "		" << implementation   << i << "();"<< std::endl;
			out << "	}" << std::endl;
		}
		out << "}" << std::endl;
	}

	~ScadTubeFile()
	{
		out.close();
	}
};

}
#endif
