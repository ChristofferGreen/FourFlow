#ifndef VolumeSliceWidget_h
#define VolumeSliceWidget_h

#define IMAGE_WIDTH 500.0
#define IMAGE_HEIGHT 500.0

#include <QWidget>
#include <math.h>
#include <list>
#include <vector>
#include <set>
#include <map>

#include "VolumeSliceView.h"

class CurrentMouseState {
public:
	enum {
		MOUSE_NOT_YET_MOVED,
		MOUSE_MOVING,
		RECTANGLE_SELECTION,
		DRAGGING_POINTS
	} States;
	int currentState, mouseNearPoint;
	bool leftButtonDown;
	QPoint mouseDownPoint, currentPoint, previousPoint, overLinePoint;
	std::vector<QPoint>::iterator iter;

	CurrentMouseState::CurrentMouseState() {
		this->currentState = MOUSE_NOT_YET_MOVED;
		this->leftButtonDown = false;
	}

	bool mouseInImage() {
		if(currentPoint.x() >= 0 && currentPoint.y() >= 0 && currentPoint.x() <= IMAGE_WIDTH && currentPoint.y() <= IMAGE_WIDTH)
			return true;
		return false;
	}

	void mouseMoved() {
		if(abs(currentPoint.x()-mouseDownPoint.x()) >= 3 || abs(currentPoint.x()-mouseDownPoint.y()) >= 3)
			this->currentState = MOUSE_MOVING;
	}

	void distanceFromLine(QPoint c, QPoint a, QPoint b, double &distanceSegment, double &distanceLine, QPoint &intersectPoint);

	bool isOverLine(std::vector<QPoint> &points) {
		if(points.size() < 2)
			return false;
		QPoint prevPoint = *points.rbegin();
		for(std::vector<QPoint>::iterator iter = points.begin(); iter != points.end(); iter++) {
			QPoint point = *iter;
			this->iter = iter;
			double distanceToSegment, distanceToLine;
			distanceFromLine(currentPoint, point, prevPoint, distanceToSegment, distanceToLine, overLinePoint);
			if(distanceToSegment < 7 && distanceToLine >= distanceToSegment)
				return true;
			prevPoint = point;
		}
		return false;
	}

	bool isMouseNearPoint(std::vector<QPoint> points) {
		for(unsigned int i = 0; i < points.size(); i++)	{
			QPoint point = points[i];
			QPoint pVec(currentPoint.x()-point.x(), currentPoint.y()-point.y());
			float distance = sqrt((float)(pVec.x()*pVec.x()+pVec.y()*pVec.y()));
			if(distance < 10.0) {
				this->mouseNearPoint = i;
				return true;
			}
		}
		return false;
	}

private:
};

class VolumeSliceWidget : public QWidget {
public:
	VolumeSliceWidget(QWidget *parent);
	int currentTimeIndex;
	typedef std::map<int, std::vector<QPoint> > pointMapType;
	pointMapType pointMap;
	std::set<int> selectedPoints;
	int hover;
	QPixmap pixmap;
	CurrentMouseState currentMouseState;
	VolumeSliceView *sliceView;

	void deleteButtonClicked();
protected:
	bool mouseDown, mouseMoved, movingPoints, rectSelection, overLine;
	QPoint firstMousePos, prevMousePos, currentMousePos;

	virtual void mouseMoveEvent ( QMouseEvent * ev );
	virtual void mousePressEvent( QMouseEvent * ev );
	virtual void mouseReleaseEvent ( QMouseEvent * ev );
	virtual void keyPressEvent ( QKeyEvent * ev );
	virtual void paintEvent( QPaintEvent * ev );
	bool mouseInImage();
	void distanceFromLine(QPoint c, QPoint a, QPoint b, double &distanceSegment, double &distanceLine, QPoint &intersectPoint);
};

#endif