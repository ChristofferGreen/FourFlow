/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRingPolygonSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "QStringList.h"
#include "QString.h"
#include "math.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/cartesian2d.hpp>
using namespace boost::geometry;

#include <math.h>

vtkStandardNewMacro(vtkRingPolygonSource);

//----------------------------------------------------------------------------
// Construct sphere with radius=0.5 and default resolution 8 in both Phi
// and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
vtkRingPolygonSource::vtkRingPolygonSource(int res)
{
  res = res < 4 ? 4 : res;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
  this->SetNumberOfInputPorts(1);
  this->Resolution = 25;
  this->Vertex2dString = 0;
}

bool vtkRingPolygonSource::isInPolygon(float x, float y) {
	boost::geometry::polygon<point_xy<double> > poly;
	for(std::vector<std::vector<double> >::iterator iter = polygon2d.vertices.begin(); iter != polygon2d.vertices.end(); iter++)
	{
		double xp = (*iter)[0];
		double yp = (*iter)[1];
		append(poly, make<point_2d>(xp, yp));
	}
	append(poly, make<point_2d>(polygon2d.vertices[0][0], polygon2d.vertices[0][1]));
	point_xy<float> point(x, y);

	return within(point, poly);
}

void vtkRingPolygonSource::convertCoordinates(double x, double y, double pos[])
{
	std::vector<double> cornerA = corners.vertices[0];
	std::vector<double> cornerB = corners.vertices[1];
	std::vector<double> cornerC = corners.vertices[2];

	double ab[3];
	ab[0] = cornerB[0]-cornerA[0];
	ab[1] = cornerB[1]-cornerA[1];
	ab[2] = cornerB[2]-cornerA[2];

	double ac[3];
	ac[0] = cornerC[0]-cornerA[0];
	ac[1] = cornerC[1]-cornerA[1];
	ac[2] = cornerC[2]-cornerA[2];

	double pushX[3];
	pushX[0] = ab[0]*x;
	pushX[1] = ab[1]*x;
	pushX[2] = ab[2]*x;

	double pushY[3];
	pushY[0] = ac[0]*y;
	pushY[1] = ac[1]*y;
	pushY[2] = ac[2]*y;

	pos[0] = cornerA[0]+pushX[0]+pushY[0];
	pos[1] = cornerA[1]+pushX[1]+pushY[1];
	pos[2] = cornerA[2]+pushX[2]+pushY[2];
}

//----------------------------------------------------------------------------
int vtkRingPolygonSource::RequestData(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector) {
	if(!Vertex2dString)
		return 1;
	if(!strlen(Vertex2dString))
		return 1;
	// get the info object
	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	vtkPolyData *probePlaneData = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

	polygon.vertices.clear();
	polygon2d.vertices.clear();
	QString vertices2dString(Vertex2dString);
	
	// find the correct time step in the data
	double requestedTimeValue = 0.0f;
	if (outInfo && outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
		requestedTimeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
	else
		return 1;

	QStringList vertices2dTimeStringList = vertices2dString.split(":");
	QString closestStep;
	float currentClosestDistance = 1000000.0f;
	float closestTime = 0.0f;
	foreach(QString timeString, vertices2dTimeStringList) {
		float currentValue = timeString.split(" ")[0].toFloat();
		float distance = fabs(currentValue-requestedTimeValue);
		if(distance < currentClosestDistance) {
			closestStep = timeString;
			currentClosestDistance = distance;
			closestTime = currentValue;
		}
	}

	QStringList vertices2dStringList = closestStep.split(" ");
	vertices2dStringList.pop_front(); // remove time data
	polygon2d.vertices.clear();
	for(QStringList::iterator iter = vertices2dStringList.begin(); iter != vertices2dStringList.end(); iter++) {
		std::vector<double> v;
		v.push_back((*iter).toDouble());
		iter++;
		v.push_back((*iter).toDouble());
		v.push_back(0.0);

		polygon2d.vertices.push_back(v);
	}

	// calculate corners
	if(probePlaneData) {
		corners.vertices.clear();
		std::vector<double> v1, v2, v3;
		double a[3];
		
		probePlaneData->GetPoint(0,a); cout << a[0] << " " << a[1] << " " << a[2] << endl;
		v1.push_back(a[0]);v1.push_back(a[1]);v1.push_back(a[2]);
		corners.vertices.push_back(v1);
		
		probePlaneData->GetPoint(100,a);
		v2.push_back(a[0]);v2.push_back(a[1]);v2.push_back(a[2]);
		corners.vertices.push_back(v2);
		
		probePlaneData->GetPoint(10100,a); // bottom left, (101*101-101)=width*height-linewidth
		v3.push_back(a[0]);v3.push_back(a[1]);v3.push_back(a[2]);
		corners.vertices.push_back(v3);
	}

	polygon.vertices.clear();
	for(std::vector<std::vector<double> >::iterator iter = polygon2d.vertices.begin(); iter != polygon2d.vertices.end(); iter++) 	{
		double x = (*iter)[0];
		double y = (*iter)[1];

		double pos[3];
		convertCoordinates(x, y, pos);
		
		std::vector<double> v;
		v.push_back(pos[0]);
		v.push_back(pos[1]);
		v.push_back(pos[2]);
		polygon.vertices.push_back(v);
	}

  // get the ouptut
	vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

	if(polygon.vertices.size() > 2) {
		vtkPoints *newPoints;
		vtkCellArray *newPolys;
		vtkIdType pts[10000];

		newPoints = vtkPoints::New();
		newPoints->Allocate(polygon.vertices.size());
		newPolys = vtkCellArray::New();
		newPolys->Allocate(1);

		unsigned int i = 0;
		for(std::vector<std::vector<double> >::iterator iter = polygon.vertices.begin(); iter != polygon.vertices.end(); iter++) {
			std::vector<double> v = *iter;
			double x[3];
			x[0] = v[0];
			x[1] = v[1];
			x[2] = v[2];
			newPoints->InsertPoint(i,x);
			i++;
		}
		
		for(int j = 0; j < polygon.vertices.size(); j++) {
			pts[j] = j;
		}
		newPolys->InsertNextCell(polygon.vertices.size(), pts);

		// add point grid
		int gridWidth = Resolution;
		int gridHeight = Resolution;
		for(unsigned int j = 0; j <= gridWidth; j++) {
			for(unsigned int k = 0; k <= gridHeight; k++) {
				double x = ((double)j)/((double)gridWidth);
				double y = ((double)k)/((double)gridHeight);
				if(isInPolygon(x,y)) {
					double pos[3];
					convertCoordinates(x, y, pos);
					newPoints->InsertPoint(i, pos);
					i++;
				}
			}
		}

		newPoints->Squeeze();
		output->SetPoints(newPoints);
		newPoints->Delete();

		newPolys->Squeeze();
		output->SetPolys(newPolys);
		newPolys->Delete();
	}
	return 1;
}

int vtkRingPolygonSource::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
	cout << "vtkTemporalStreamTracer::RequestUpdateExtent" << endl;
	return 1;
}

//----------------------------------------------------------------------------
void vtkRingPolygonSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "nothing here\n";
}

//----------------------------------------------------------------------------
int vtkRingPolygonSource::RequestInformation(vtkInformation *vtkNotUsed(request),
	vtkInformationVector **vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0); // get the info object
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
               this->Center[0] - 0.5,
               this->Center[0] + 0.5,
               this->Center[1] - 0.5,
               this->Center[1] + 0.5,
               this->Center[2] - 0.5,
               this->Center[2] + 0.5);
	// update time data
	QString verticesCornerString(Vertex2dString);
	QStringList verticesCornerList = verticesCornerString.split("|");
	QString vertices2dString = verticesCornerList[0];
	QStringList vertices2dTimeStringList = vertices2dString.split(":");
	int size = vertices2dTimeStringList.size();
	double *timeValues = new double[size];
	for(int i = 0; i < size; i++) {
		QString timeString = vertices2dTimeStringList[i];
		float currentValue = timeString.split(" ")[0].toFloat();
		timeValues[i] = currentValue;
	}
	double timeRange[2];
	timeRange[0] = timeValues[0];
	timeRange[1] = timeValues[size-1];
	outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
	outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeValues, size);
	delete timeValues;

  return 1;
}
