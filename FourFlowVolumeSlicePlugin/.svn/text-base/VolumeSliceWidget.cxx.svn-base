#include "VolumeSliceWidget.h"
#include <QPainter.h>
#include <QMouseEvent>

#include <vtkSMProxy.h>
#include <pqPVApplicationCore.h>
#include <pqAnimationScene.h>
#include <pqAnimationManager.h>

VolumeSliceWidget::VolumeSliceWidget(QWidget *parent) : QWidget(parent) {
	mouseDown = false;
	mouseMoved = false;
	movingPoints = false;
	rectSelection = false;
	overLine = false;
	hover = -1;
	this->setMouseTracking(true);
	currentTimeIndex = 0;
}

void VolumeSliceWidget::deleteButtonClicked() {
	std::vector<QPoint> delList;
	foreach(int p, selectedPoints)
		delList.push_back(this->pointMap[this->currentTimeIndex][p]);
	while(!delList.empty())
	{
		QPoint point = *delList.begin();
		delList.erase(delList.begin());
		for(std::vector<QPoint>::iterator iter = this->pointMap[this->currentTimeIndex].begin(); iter != this->pointMap[this->currentTimeIndex].end(); iter++)
		{
			if(point == *iter)
			{
				this->pointMap[this->currentTimeIndex].erase(iter);
				break;
			}
		}
	}
	selectedPoints.clear();
	this->repaint();
	this->sliceView->updatePolygon();
}

void VolumeSliceWidget::keyPressEvent ( QKeyEvent * event ) {
	if(event->key() == Qt::Key_Delete) {
		this->deleteButtonClicked();	
	}
	if(event->key()==Qt::Key_Left) {
		cout << "key left" << endl;
		pqAnimationManager *animationManager = pqPVApplicationCore::instance()->animationManager();
		pqAnimationScene *animationScene = animationManager->getActiveScene();
		animationScene->setCacheGeometrySetting(false);
		animationScene->getProxy()->InvokeCommand("GoToPrevious");
		animationScene->setCacheGeometrySetting(true);
	}
	if(event->key()==Qt::Key_Right) {
		cout << "key right" << endl;
		pqAnimationManager *animationManager = pqPVApplicationCore::instance()->animationManager();
		pqAnimationScene *animationScene = animationManager->getActiveScene();
		animationScene->setCacheGeometrySetting(false);
		animationScene->getProxy()->InvokeCommand("GoToNext");
		animationScene->setCacheGeometrySetting(true);
	}
	this->repaint();
}

void VolumeSliceWidget::mousePressEvent( QMouseEvent * ev ) {
	// initial mouse down setup
	setFocus();
	//grabKeyboard();
	if(!currentMouseState.mouseInImage())
		QWidget::mousePressEvent(ev);

	// set mouse state
	currentMouseState.previousPoint = ev->pos();
	currentMouseState.currentPoint = ev->pos();
	currentMouseState.mouseDownPoint  = ev->pos();
	currentMouseState.currentState = CurrentMouseState::MOUSE_NOT_YET_MOVED;
	currentMouseState.leftButtonDown = ev->button() == Qt::LeftButton;

	// select point
	if(currentMouseState.isMouseNearPoint(this->pointMap[this->currentTimeIndex])) {
		if(!selectedPoints.count(currentMouseState.mouseNearPoint))
			selectedPoints.clear();
		selectedPoints.insert(currentMouseState.mouseNearPoint);
	}
}

void VolumeSliceWidget::mouseMoveEvent( QMouseEvent * ev ) {
	// set mouse state
	currentMouseState.currentPoint = ev->pos();
	bool mouseInImage = currentMouseState.mouseInImage();
	bool nearPoint = currentMouseState.isMouseNearPoint(this->pointMap[this->currentTimeIndex]);
	bool leftButton = currentMouseState.leftButtonDown;
	bool draggingPoints = currentMouseState.currentState == CurrentMouseState::DRAGGING_POINTS;
	bool rectangleSelect = currentMouseState.currentState == CurrentMouseState::RECTANGLE_SELECTION;

	// move selected points
	if(draggingPoints || (!rectangleSelect && (mouseInImage && nearPoint && leftButton))) {
		currentMouseState.currentState = CurrentMouseState::DRAGGING_POINTS;
		float xDiff = currentMouseState.currentPoint.x()-currentMouseState.previousPoint.x();
		float yDiff = currentMouseState.currentPoint.y()-currentMouseState.previousPoint.y();
		foreach(int p, selectedPoints) {
			this->pointMap[this->currentTimeIndex][p].setX(this->pointMap[this->currentTimeIndex][p].x()+xDiff);
			this->pointMap[this->currentTimeIndex][p].setY(this->pointMap[this->currentTimeIndex][p].y()+yDiff);
		}
	}

	// rectangle select if mouse down over a few pixels (to give chanse for adding new points when mouse has only moved a little)
	if(leftButton) {
		int xx = currentMouseState.mouseDownPoint.x()-currentMouseState.currentPoint.x();
		int yy = currentMouseState.mouseDownPoint.y()-currentMouseState.currentPoint.y();
		float distanceToMouseDown = sqrt((float)(xx*xx+yy*yy));
		if(!draggingPoints && (mouseInImage && !nearPoint) && distanceToMouseDown > 15.0f) {
			currentMouseState.currentState = CurrentMouseState::RECTANGLE_SELECTION;
		}
	}

	// set mouse state
	currentMouseState.previousPoint = currentMouseState.currentPoint;
	this->repaint();
}

void VolumeSliceWidget::mouseReleaseEvent( QMouseEvent * ev ) {
	if(currentMouseState.mouseInImage()) {
		if(currentMouseState.currentState == CurrentMouseState::MOUSE_NOT_YET_MOVED) { // possibly add point
			bool overLine = currentMouseState.isOverLine(this->pointMap[this->currentTimeIndex]);
			bool nearPoint = currentMouseState.isMouseNearPoint(this->pointMap[this->currentTimeIndex]);
			if(currentMouseState.leftButtonDown && !overLine && !nearPoint) { // add new point
				this->selectedPoints.clear();
				this->pointMap[this->currentTimeIndex].push_back(ev->pos());
			}
			else if(currentMouseState.leftButtonDown && overLine && !nearPoint) { // add new point over line
				this->selectedPoints.clear();
				this->pointMap[this->currentTimeIndex].insert(currentMouseState.iter, currentMouseState.overLinePoint);
			}
		}
		else if(currentMouseState.currentState == CurrentMouseState::RECTANGLE_SELECTION) { // 
			selectedPoints.clear();
			for(unsigned int i = 0; i < this->pointMap[this->currentTimeIndex].size(); i++) {
				QPoint point = this->pointMap[this->currentTimeIndex][i];
				if(point.x() > currentMouseState.mouseDownPoint.x() && point.x() < currentMouseState.currentPoint.x() && 
					point.y() > currentMouseState.mouseDownPoint.y() && point.y() < currentMouseState.currentPoint.y()) {
					selectedPoints.insert(i);
				}
				if(point.x() < currentMouseState.mouseDownPoint.x() && point.x() > currentMouseState.currentPoint.x() && 
					point.y() < currentMouseState.mouseDownPoint.y() && point.y() > currentMouseState.currentPoint.y()) {
					selectedPoints.insert(i);
				}
				if(point.x() > currentMouseState.mouseDownPoint.x() && point.x() < currentMouseState.currentPoint.x() && 
					point.y() < currentMouseState.mouseDownPoint.y() && point.y() > currentMouseState.currentPoint.y()) {
					selectedPoints.insert(i);
				}
				if(point.x() < currentMouseState.mouseDownPoint.x() && point.x() > currentMouseState.currentPoint.x() && 
					point.y() > currentMouseState.mouseDownPoint.y() && point.y() < currentMouseState.currentPoint.y()) {
					selectedPoints.insert(i);
				}
			}
		}
	}

	if(ev->button() == Qt::LeftButton)
		currentMouseState.leftButtonDown = false;
	currentMouseState.currentState = CurrentMouseState::MOUSE_NOT_YET_MOVED;
	this->repaint();
	this->sliceView->updatePolygon();
}

void VolumeSliceWidget::paintEvent( QPaintEvent * ev ) {

	QWidget::paintEvent(ev);
	QPainter painter(this);
	painter.drawPixmap(0,0,pixmap);
	if(this->pointMap[this->currentTimeIndex].empty())
		return;
	painter.setPen(Qt::red);
	QPoint prev = *this->pointMap[this->currentTimeIndex].begin();
	foreach(const QPoint &point, this->pointMap[this->currentTimeIndex]) {
		painter.drawLine(prev, point);
		prev = point;
	}
	painter.drawLine(*this->pointMap[this->currentTimeIndex].begin(), *this->pointMap[this->currentTimeIndex].rbegin());
	for(unsigned int i = 0; i < this->pointMap[this->currentTimeIndex].size(); i++) {
		QPoint point = this->pointMap[this->currentTimeIndex][i];
		if(this->selectedPoints.count(i))
			painter.setPen(QColor(150,150,255));
		else
			painter.setPen(Qt::green);
		painter.drawRect(point.x()-4, point.y()-4, 8, 8);
	}

	if(currentMouseState.currentState == CurrentMouseState::RECTANGLE_SELECTION) {
		painter.setPen(QColor(150,150,255));
		painter.drawRect(QRect(currentMouseState.mouseDownPoint, currentMouseState.currentPoint));
	}
	else if(currentMouseState.isMouseNearPoint(this->pointMap[this->currentTimeIndex])) {
		painter.setPen(QColor(200,150,255));
		QPoint point = this->pointMap[this->currentTimeIndex][currentMouseState.mouseNearPoint];
		painter.drawRect(point.x()-4, point.y()-4, 8, 8);
	}
	else if(currentMouseState.isOverLine(this->pointMap[this->currentTimeIndex])) {
		painter.setPen(QColor(150,150,255));
		painter.drawRect(currentMouseState.overLinePoint.x()-2, currentMouseState.overLinePoint.y()-2, 4, 4);
	}
}
void CurrentMouseState::distanceFromLine(QPoint c, QPoint a, QPoint b, double &distanceSegment, double &distanceLine, QPoint &intersectPoint) {
// c = point, a = endpoint 1, b = endpoint 2

	//
	// find the distance from the point (cx,cy) to the line
	// determined by the points (ax,ay) and (bx,by)
	//
	// distanceSegment = distance from the point to the line segment
	// distanceLine = distance from the point to the line (assuming
	//					infinite extent in both directions
	//

/*

Subject 1.02: How do I find the distance from a point to a line?


    Let the point be C (Cx,Cy) and the line be AB (Ax,Ay) to (Bx,By).
    Let P be the point of perpendicular projection of C on AB.  The parameter
    r, which indicates P's position along AB, is computed by the dot product 
    of AC and AB divided by the square of the length of AB:
    
    (1)     AC dot AB
        r = ---------  
            ||AB||^2
    
    r has the following meaning:
    
        r=0      P = A
        r=1      P = B
        r<0      P is on the backward extension of AB
        r>1      P is on the forward extension of AB
        0<r<1    P is interior to AB
    
    The length of a line segment in d dimensions, AB is computed by:
    
        L = sqrt( (Bx-Ax)^2 + (By-Ay)^2 + ... + (Bd-Ad)^2)

    so in 2D:   
    
        L = sqrt( (Bx-Ax)^2 + (By-Ay)^2 )
    
    and the dot product of two vectors in d dimensions, U dot V is computed:
    
        D = (Ux * Vx) + (Uy * Vy) + ... + (Ud * Vd)
    
    so in 2D:   
    
        D = (Ux * Vx) + (Uy * Vy) 
    
    So (1) expands to:
    
            (Cx-Ax)(Bx-Ax) + (Cy-Ay)(By-Ay)
        r = -------------------------------
                          L^2

    The point P can then be found:

        Px = Ax + r(Bx-Ax)
        Py = Ay + r(By-Ay)

    And the distance from A to P = r*L.

    Use another parameter s to indicate the location along PC, with the 
    following meaning:
           s<0      C is left of AB
           s>0      C is right of AB
           s=0      C is on AB

    Compute s as follows:

            (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
        s = -----------------------------
                        L^2


    Then the distance from C to P = |s|*L.

*/


	double r_numerator = (c.x()-a.x())*(b.x()-a.x()) + (c.y()-a.y())*(b.y()-a.y());
	double r_denomenator = (b.x()-a.x())*(b.x()-a.x()) + (b.y()-a.y())*(b.y()-a.y());
	double r = r_numerator / r_denomenator;
//
    double px = a.x() + r*(b.x()-a.x());
    double py = a.y() + r*(b.y()-a.y());
	intersectPoint.setX(px);
	intersectPoint.setY(py);
//     
    double s =  ((a.y()-c.y())*(b.x()-a.x())-(a.x()-c.x())*(b.y()-a.y()) ) / r_denomenator;

	distanceLine = fabs(s)*sqrt(r_denomenator);

//
// (xx,yy) is the point on the lineSegment closest to (cx,cy)
//
	double xx = px;
	double yy = py;

	if ( (r >= 0) && (r <= 1) )
	{
		distanceSegment = distanceLine;
	}
	else
	{

		double dist1 = (c.x()-a.x())*(c.x()-a.x()) + (c.y()-a.y())*(c.y()-a.y());
		double dist2 = (c.x()-b.x())*(c.x()-b.x()) + (c.y()-b.y())*(c.y()-b.y());
		if (dist1 < dist2)
		{
			xx = a.x();
			yy = a.y();
			distanceSegment = sqrt(dist1);
		}
		else
		{
			xx = b.x();
			yy = b.y();
			distanceSegment = sqrt(dist2);
		}


	}

	return;
}