#ifndef __vtkFourFlowCollectorStructs_h
#define __vtkFourFlowCollectorStructs_h

#include <vector>
using namespace std;

struct Point {
	double pos[3];
	int id;
};

struct Line {
	Point p1;
	Point p2;
};

struct LineCollection {
	vector<Line> lines;
};

struct PointBuffer {
	vector<Point> previousPoints;
	vector<Point> currentPoints;
};

#endif


