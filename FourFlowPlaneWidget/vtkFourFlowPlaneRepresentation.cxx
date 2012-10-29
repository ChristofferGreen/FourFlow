#include "vtkFourFlowPlaneRepresentation.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLineSource.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkCallbackCommand.h"
#include "vtkBox.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkWindow.h"
#include "vtkObjectFactory.h"
#include "vtkVectorText.h"
#include "vtkFollower.h"
#include "vtkCellPicker.h"
#include "vtkPolyDataMapper.h"
#include "vtkConeSource.h"
#include "vtkPlaneSource.h"
#include "vtkAssemblyPath.h"
#include "vtkTransform.h"
#include "vtkPlane.h"

#ifndef isnan
inline bool isnan(double x) {
    return x != x;
}
#endif

vtkStandardNewMacro(vtkFourFlowPlaneRepresentation);

vtkCxxSetObjectMacro(vtkFourFlowPlaneRepresentation,HandleRepresentation,vtkPointHandleRepresentation3D);

//----------------------------------------------------------------------------
vtkFourFlowPlaneRepresentation::vtkFourFlowPlaneRepresentation() {
	//---------------------------------------------------------------------------------------------
	this->PlaneSource = vtkPlaneSource::New();
	this->PlaneSource->SetCenter(0,0,0);
	this->PlaneSource->SetPoint1(0,0.05,0);
	this->PlaneSource->SetPoint2(-0.05,0,0);
	//this->PlaneSource->SetNormal(0,0,1);
	this->PlaneSource->SetResolution(100, 100);

	this->Corner1 = vtkSphereSource::New();
	this->Corner1->SetThetaResolution(16);
	this->Corner1->SetPhiResolution(8);
	this->CornerMapper1 = vtkPolyDataMapper::New();
	this->CornerMapper1->SetInput(this->Corner1->GetOutput());
	this->CornerActor1 = vtkActor::New();
	this->CornerActor1->GetProperty()->SetColor(0.3,0.3,1);
	this->CornerActor1->SetMapper(this->CornerMapper1);

	this->Corner2 = vtkSphereSource::New();
	this->Corner2->SetThetaResolution(16);
	this->Corner2->SetPhiResolution(8);
	this->CornerMapper2 = vtkPolyDataMapper::New();
	this->CornerMapper2->SetInput(this->Corner2->GetOutput());
	this->CornerActor2 = vtkActor::New();
	this->CornerActor2->GetProperty()->SetColor(0.3,0.3,1);
	this->CornerActor2->SetMapper(this->CornerMapper2);

	this->Corner3 = vtkSphereSource::New();
	this->Corner3->SetThetaResolution(16);
	this->Corner3->SetPhiResolution(8);
	this->CornerMapper3 = vtkPolyDataMapper::New();
	this->CornerMapper3->SetInput(this->Corner3->GetOutput());
	this->CornerActor3 = vtkActor::New();
	this->CornerActor3->GetProperty()->SetColor(0.3,0.3,1);
	this->CornerActor3->SetMapper(this->CornerMapper3);

	this->Corner4 = vtkSphereSource::New();
	this->Corner4->SetThetaResolution(16);
	this->Corner4->SetPhiResolution(8);
	this->CornerMapper4 = vtkPolyDataMapper::New();
	this->CornerMapper4->SetInput(this->Corner4->GetOutput());
	this->CornerActor4 = vtkActor::New();
	this->CornerActor4->GetProperty()->SetColor(0.3,0.3,1);
	this->CornerActor4->SetMapper(this->CornerMapper4);

	// Create the + plane normal
	this->LineSource1 = vtkLineSource::New();
	//this->LineSource1->SetResolution(1);
	this->LineMapper1 = vtkPolyDataMapper::New();
	this->LineMapper1->SetInput(this->LineSource1->GetOutput());
	this->LineActor1 = vtkActor::New();
	this->LineActor1->SetMapper(this->LineMapper1);

	this->ConeSource = vtkConeSource::New();
	this->ConeSource->SetResolution(12);
	this->ConeSource->SetAngle(25.0);
	this->ConeMapper = vtkPolyDataMapper::New();
	this->ConeMapper->SetInput(this->ConeSource->GetOutput());
	this->ConeActor = vtkActor::New();
	this->ConeActor->GetProperty()->SetColor(1,0,0);
	this->ConeActor->SetMapper(this->ConeMapper);

	// Create the - plane normal
	this->LineSource2 = vtkLineSource::New();
	//this->LineSource2->SetResolution(1);
	this->LineMapper2 = vtkPolyDataMapper::New();
	this->LineMapper2->SetInput(this->LineSource2->GetOutput());
	this->LineActor2 = vtkActor::New();
	this->LineActor2->SetMapper(this->LineMapper2);

	this->ConeSource2 = vtkConeSource::New();
	this->ConeSource2->SetResolution(12);
	this->ConeSource2->SetAngle(25.0);
	this->ConeMapper2 = vtkPolyDataMapper::New();
	this->ConeMapper2->SetInput(this->ConeSource2->GetOutput());
	this->ConeActor2 = vtkActor::New();
	this->ConeActor2->GetProperty()->SetColor(1,0,0);
	this->ConeActor2->SetMapper(this->ConeMapper2);

	// Create the origin handle
	this->Sphere = vtkSphereSource::New();
	this->Sphere->SetThetaResolution(16);
	this->Sphere->SetPhiResolution(8);
	this->SphereMapper = vtkPolyDataMapper::New();
	this->SphereMapper->SetInput(this->Sphere->GetOutput());
	this->SphereActor = vtkActor::New();
	this->SphereActor->SetMapper(this->SphereMapper);
	
	this->Transform = vtkTransform::New();

	//Manage the picking stuff
	this->Picker = vtkCellPicker::New();
	this->Picker->SetTolerance(0.005);
	this->Picker->AddPickList(this->LineActor1);
	this->Picker->AddPickList(this->LineActor2);
	this->Picker->AddPickList(this->ConeActor);
	this->Picker->AddPickList(this->ConeActor2);
	this->Picker->AddPickList(this->SphereActor);
	this->Picker->AddPickList(this->CornerActor1);
	this->Picker->AddPickList(this->CornerActor2);
	this->Picker->AddPickList(this->CornerActor3);
	this->Picker->AddPickList(this->CornerActor4);
	this->Picker->PickFromListOn();
	//---------------------------------------------------------------------------------------------


  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  // By default, use one of these handles
  this->HandleRepresentation  = vtkPointHandleRepresentation3D::New();
  this->HandleRepresentation->AllOff();
  this->HandleRepresentation->SetHotSpotSize(1.0);
  this->HandleRepresentation->SetPlaceFactor(1.0);
  this->HandleRepresentation->TranslationModeOn();
  this->Point1Representation = NULL;
  this->Point2Representation = NULL;
  this->LineHandleRepresentation = NULL;
  this->InstantiateHandleRepresentation();

  // Miscellaneous parameters
  this->Tolerance = 5;
  this->Placed = 0;

  // Represent the line
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(5);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInput(this->LineSource->GetOutput());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  // Create the handles
  this->Handle = new vtkActor* [2];
  this->HandleMapper = new vtkPolyDataMapper* [2];
  this->HandleGeometry = new vtkSphereSource* [2];
  for (int i=0; i<2; i++)
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = vtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
    }

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Pass the initial properties to the actors.
  this->Handle[0]->SetProperty(this->EndPointProperty);
  this->Point1Representation->SetProperty(this->EndPointProperty);
  this->Handle[1]->SetProperty(this->EndPoint2Property);
  this->Point2Representation->SetProperty(this->EndPoint2Property);
  this->LineHandleRepresentation->SetProperty(this->EndPointProperty);
  this->LineActor->SetProperty(this->LineProperty);

  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;
  this->PlaceFactor = 1.0; //overload parent's value

  this->ClampToBounds = 0;

  // The bounding box
  this->BoundingBox = vtkBox::New();

  this->RepresentationState = vtkFourFlowPlaneRepresentation::Outside;
  
  // Initial creation of the widget, serves to initialize it.
  // Call PlaceWidget() LAST in the constructor, as this method depends on ivar
  // values.
  this->PlaceWidget(bounds);

}

//----------------------------------------------------------------------------
vtkFourFlowPlaneRepresentation::~vtkFourFlowPlaneRepresentation()
{  
  if ( this->HandleRepresentation )
    {
    this->HandleRepresentation->Delete();
    }
  if ( this->Point1Representation )
    {
    this->Point1Representation->Delete();
    }
  if ( this->Point2Representation )
    {
    this->Point2Representation->Delete();
    }
  if ( this->LineHandleRepresentation )
    {
    this->LineHandleRepresentation->Delete();
    }

  this->LineActor->Delete();
  this->LineMapper->Delete();
  this->LineSource->Delete();

  for (int i=0; i<2; i++)
    {
    this->HandleGeometry[i]->Delete();
    this->HandleMapper[i]->Delete();
    this->Handle[i]->Delete();
    }
  delete [] this->Handle;
  delete [] this->HandleMapper;
  delete [] this->HandleGeometry;

  this->EndPointProperty->Delete();
  this->SelectedEndPointProperty->Delete();
  this->EndPoint2Property->Delete();
  this->SelectedEndPoint2Property->Delete();
  this->LineProperty->Delete();
  this->SelectedLineProperty->Delete();

  this->BoundingBox->Delete();
}

//----------------------------------------------------------------------
void vtkFourFlowPlaneRepresentation::InstantiateHandleRepresentation()
{
  if ( ! this->Point1Representation )
    {
    this->Point1Representation = this->HandleRepresentation->NewInstance();
    this->Point1Representation->ShallowCopy(this->HandleRepresentation);
    }
  
  if ( ! this->Point2Representation )
    {
    this->Point2Representation = this->HandleRepresentation->NewInstance();
    this->Point2Representation->ShallowCopy(this->HandleRepresentation);
    }

  if ( ! this->LineHandleRepresentation )
    {
    this->LineHandleRepresentation = this->HandleRepresentation->NewInstance();
    this->LineHandleRepresentation->ShallowCopy(this->HandleRepresentation);
    }
}
  
void vtkFourFlowPlaneRepresentation::SetOrigin(double pos[3]) {
	this->PlaneSource->SetOrigin(pos);
}

void vtkFourFlowPlaneRepresentation::SetPoint1(double pos[3]) {
	this->PlaneSource->SetPoint1(pos);
}

void vtkFourFlowPlaneRepresentation::SetPoint2(double pos[3]) {
	this->PlaneSource->SetPoint2(pos);
}

double *vtkFourFlowPlaneRepresentation::GetOrigin() {
	return this->PlaneSource->GetOrigin();
}

double *vtkFourFlowPlaneRepresentation::GetPoint1() {
	return this->PlaneSource->GetPoint1();
}

double *vtkFourFlowPlaneRepresentation::GetPoint2() {
	return this->PlaneSource->GetPoint2();
}
  
void vtkFourFlowPlaneRepresentation::SetRenderer(vtkRenderer* ren) {
	this->HandleRepresentation->SetRenderer(ren);
	this->Point1Representation->SetRenderer(ren);
	this->Point2Representation->SetRenderer(ren);
	this->LineHandleRepresentation->SetRenderer(ren);
	this->Superclass::SetRenderer(ren);
}

void vtkFourFlowPlaneRepresentation::StartWidgetInteraction(double e[2]) {
	this->StartEventPosition[0] = e[0];
	this->StartEventPosition[1] = e[1];
	this->StartEventPosition[2] = 0.0;

	this->LastEventPosition[0] = e[0];
	this->LastEventPosition[1] = e[1];
	this->LastEventPosition[2] = 0.0;
}

void vtkFourFlowPlaneRepresentation::WidgetInteraction(double e[2]) {
	double focalPoint[4], pickPoint[4], prevPickPoint[4];
	double z, vpn[3];

	vtkCamera *camera = this->Renderer->GetActiveCamera();
	if ( !camera )
		return;
	

	// Compute the two points defining the motion vector
	double pos[3];
	this->Picker->GetPickPosition(pos);
	vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, pos[0], pos[1], pos[2], focalPoint);
	z = focalPoint[2];
	vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,this->LastEventPosition[0], this->LastEventPosition[1], z, prevPickPoint);
	vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

	// Process the motion
	if ( this->InteractionState == vtkFourFlowPlaneRepresentation::MovingPlane )
		this->TranslatePlane(prevPickPoint, pickPoint);
	else if ( this->InteractionState == vtkFourFlowPlaneRepresentation::MovingCorner1 )
		this->MovePoint1(prevPickPoint, pickPoint);
	else if ( this->InteractionState == vtkFourFlowPlaneRepresentation::MovingCorner2 )
		this->MovePoint1(prevPickPoint, pickPoint);
	else if ( this->InteractionState == vtkFourFlowPlaneRepresentation::MovingCorner3 )
		this->MovePoint1(pickPoint, prevPickPoint);
	else if ( this->InteractionState == vtkFourFlowPlaneRepresentation::MovingCorner4 )
		this->MovePoint1(pickPoint, prevPickPoint);
	else if ( this->InteractionState == vtkFourFlowPlaneRepresentation::MovingOrigin )
		this->TranslatePlane(prevPickPoint, pickPoint);
	else if ( this->InteractionState == vtkFourFlowPlaneRepresentation::Pushing )
		this->Push(prevPickPoint, pickPoint);
	else if ( this->InteractionState == vtkFourFlowPlaneRepresentation::Rotating ) {
		camera->GetViewPlaneNormal(vpn);
		this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
	}

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

void vtkFourFlowPlaneRepresentation::Push(double *p1, double *p2) {
	double v[3];
	v[0] = p2[0] - p1[0];
	v[1] = p2[1] - p1[1];
	v[2] = p2[2] - p1[2];

	this->PlaneSource->Push( vtkMath::Dot(v,this->PlaneSource->GetNormal()) );
	this->SetOrigin(this->PlaneSource->GetOrigin());
	this->BuildRepresentation();
}


void vtkFourFlowPlaneRepresentation::PositionHandles() {
	double *o = this->PlaneSource->GetOrigin();
	double *pt1 = this->PlaneSource->GetPoint1();
	double *pt2 = this->PlaneSource->GetPoint2();
	this->Corner1->SetCenter(o);
	this->Corner2->SetCenter(pt1);
	this->Corner3->SetCenter(pt2);
	double x[3];
	x[0] = pt1[0] + pt2[0] - o[0];
	x[1] = pt1[1] + pt2[1] - o[1];
	x[2] = pt1[2] + pt2[2] - o[2];
	this->Corner4->SetCenter(x);
}

//----------------------------------------------------------------------------
void vtkFourFlowPlaneRepresentation::PlaceWidget(double bds[6]) {
	int i;
	double bounds[6], origin[3];

	this->AdjustBounds(bds, bounds, origin);

	this->PlaneSource->SetOrigin(origin[0],bounds[2],bounds[4]);
	this->PlaneSource->SetPoint1(origin[0],bounds[3],bounds[4]);
	this->PlaneSource->SetPoint2(origin[0],bounds[2],bounds[5]);

	this->PlaneSource->Update();

	// Position the handles at the end of the planes
	this->PositionHandles();


	this->LineSource1->SetPoint1(this->PlaneSource->GetCenter());
	this->PlaneSource->SetNormal(1,0,0);
	this->LineSource1->SetPoint2(1,0,0);

	for (i=0; i<6; i++)
	{
	this->InitialBounds[i] = bounds[i];
	}

	this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
						 (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
						 (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

	this->ValidPick = 1; // since we have positioned the widget successfully  
	this->BuildRepresentation();
}


//----------------------------------------------------------------------------
int vtkFourFlowPlaneRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify)) {
	this->Picker->Pick(X,Y,0.0,this->Renderer);
	vtkAssemblyPath *path = this->Picker->GetPath();

	if ( path == NULL ) { //not picking this widget
		this->SetRepresentationState(vtkFourFlowPlaneRepresentation::Outside);
		this->InteractionState = vtkFourFlowPlaneRepresentation::Outside;
		return this->InteractionState;
	}

	// Something picked, continue
	this->ValidPick = 1;
	
	// Depending on the interaction state (set by the widget) we modify
	// this state based on what is picked.
	vtkProp *prop = path->GetFirstNode()->GetViewProp();
	if ( prop == this->ConeActor || prop == this->ConeActor2  )
		this->InteractionState = vtkFourFlowPlaneRepresentation::Rotating;
	else if ( prop == this->LineActor1 || prop == this->LineActor2 )
		this->InteractionState = vtkFourFlowPlaneRepresentation::Pushing;
	else if ( prop == this->CornerActor1 )
		this->InteractionState = vtkFourFlowPlaneRepresentation::MovingCorner1;
	else if ( prop == this->CornerActor2 )
		this->InteractionState = vtkFourFlowPlaneRepresentation::MovingCorner2;
	else if ( prop == this->CornerActor3 )
		this->InteractionState = vtkFourFlowPlaneRepresentation::MovingCorner3;
	else if ( prop == this->CornerActor4 )
		this->InteractionState = vtkFourFlowPlaneRepresentation::MovingCorner4;
	else if ( prop == this->SphereActor )
		this->InteractionState = vtkFourFlowPlaneRepresentation::MovingOrigin;
	else
		this->InteractionState = vtkFourFlowPlaneRepresentation::Outside;
	
	this->SetRepresentationState(this->InteractionState);

	return this->InteractionState;
}

void vtkFourFlowPlaneRepresentation::SetRepresentationState(int state) {
  this->RepresentationState = state;
  this->Modified();
}

void vtkFourFlowPlaneRepresentation::MovePoint1(double *p1, double *p2) {
  double *o = this->PlaneSource->GetOrigin();
  double *pt1 = this->PlaneSource->GetPoint1();
  double *pt2 = this->PlaneSource->GetPoint2();
  double *center = this->Sphere->GetCenter();

  if(isnan(o[0]) || isnan(o[1]) || isnan(o[2]) ||
	  isnan(pt1[0]) || isnan(pt1[1]) || isnan(pt1[2]) ||
	  isnan(pt2[0]) || isnan(pt2[1]) || isnan(pt2[2]))
	return;

  //Get the vector of motion
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // The point opposite the origin (pt3) stays fixed
  double pt3[3];
  pt3[0] = o[0] + (pt1[0] - o[0]) + (pt2[0] - o[0]);
  pt3[1] = o[1] + (pt1[1] - o[1]) + (pt2[1] - o[1]);
  pt3[2] = o[2] + (pt1[2] - o[2]) + (pt2[2] - o[2]);

  // Define vectors from point pt3
  double p13[3], p23[3];
  p13[0] = pt1[0] - pt3[0];
  p13[1] = pt1[1] - pt3[1];
  p13[2] = pt1[2] - pt3[2];
  p23[0] = pt2[0] - pt3[0];
  p23[1] = pt2[1] - pt3[1];
  p23[2] = pt2[2] - pt3[2];

  double vN = vtkMath::Norm(v);
  double n13 = vtkMath::Norm(p13);
  double n23 = vtkMath::Norm(p23);

  // Project v onto these vector to determine the amount of motion
  // Scale it by the relative size of the motion to the vector length
  double d1 = (vN/n13) * vtkMath::Dot(v,p13) / (vN*n13);
  double d2 = (vN/n23) * vtkMath::Dot(v,p23) / (vN*n23);

  double point1[3], point2[3], origin[3];

  for (int i=0; i<3; i++) {
    point1[i] = pt3[i] + (1.0+d1)*p13[i] - d1*p23[i];
    point2[i] = pt3[i] + (1.0+d1)*p23[i] - d1*p13[i];
    origin[i] = pt3[i] + (1.0+d1)*p13[i] + (1.0+d1)*p23[i];
  }

  if(!isnan(origin[0]) && !isnan(origin[1]) && !isnan(origin[2]) &&
	  !isnan(point1[0]) && !isnan(point1[1]) && !isnan(point1[2]) &&
	  !isnan(point2[0]) && !isnan(point2[1]) && !isnan(point2[2]))
  {
	this->PlaneSource->SetOrigin(origin);
	this->PlaneSource->SetPoint1(point1);
	this->PlaneSource->SetPoint2(point2);
	this->PlaneSource->Update();
	this->PositionHandles();
  }
}

void vtkFourFlowPlaneRepresentation::Rotate(double X, double Y, double *p1, double *p2, double *vpn) {
	double v[3]; //vector of motion
	double axis[3]; //axis of rotation
	double theta; //rotation angle

	// mouse motion vector in world space
	v[0] = p2[0] - p1[0];
	v[1] = p2[1] - p1[1];
	v[2] = p2[2] - p1[2];

	double *origin = this->PlaneSource->GetCenter();
	double *normal = this->PlaneSource->GetNormal();

	// Create axis of rotation and angle of rotation
	vtkMath::Cross(vpn,v,axis);
	if ( vtkMath::Normalize(axis) == 0.0 )
		return;
	int *size = this->Renderer->GetSize();
	double l2 = (X-this->LastEventPosition[0])*(X-this->LastEventPosition[0]) + (Y-this->LastEventPosition[1])*(Y-this->LastEventPosition[1]);
	theta = 360.0 * sqrt(l2/(size[0]*size[0]+size[1]*size[1]));

	//Manipulate the transform to reflect the rotation
	this->Transform->Identity();
	this->Transform->Translate(origin[0],origin[1],origin[2]);
	this->Transform->RotateWXYZ(theta,axis);
	this->Transform->Translate(-origin[0],-origin[1],-origin[2]);

	//Set the new normal
	double nNew[3];
	this->Transform->TransformNormal(normal,nNew);
	this->PlaneSource->SetNormal(nNew);

	this->BuildRepresentation();
}

void vtkFourFlowPlaneRepresentation::TranslatePlane(double *p1, double *p2) { // Loop through all points and translate them
	//Get the motion vector
	double v[3];
	v[0] = p2[0] - p1[0];
	v[1] = p2[1] - p1[1];
	v[2] = p2[2] - p1[2];

	//Translate the plane
	double oNew[3];
	double *origin = this->PlaneSource->GetCenter();
	oNew[0] = origin[0] + v[0];
	oNew[1] = origin[1] + v[1];
	oNew[2] = origin[2] + v[2];
	this->PlaneSource->SetCenter(oNew);

	this->BuildRepresentation();
}

double *vtkFourFlowPlaneRepresentation::GetBounds() {
	this->BuildRepresentation();
	this->BoundingBox->SetBounds(this->CornerActor1->GetBounds());
	this->BoundingBox->SetBounds(this->CornerActor2->GetBounds());
	this->BoundingBox->SetBounds(this->CornerActor3->GetBounds());
	this->BoundingBox->SetBounds(this->CornerActor4->GetBounds());
	this->BoundingBox->SetBounds(this->ConeActor->GetBounds());
	this->BoundingBox->SetBounds(this->ConeActor2->GetBounds());
	this->BoundingBox->SetBounds(this->LineActor1->GetBounds());
	this->BoundingBox->SetBounds(this->LineActor2->GetBounds());
	this->BoundingBox->SetBounds(this->SphereActor->GetBounds());
	return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------------
void vtkFourFlowPlaneRepresentation::CreateDefaultProperties()
{
  // Endpoint properties
  this->EndPointProperty = vtkProperty::New();
  this->EndPointProperty->SetColor(1,1,1);

  this->SelectedEndPointProperty = vtkProperty::New();
  this->SelectedEndPointProperty->SetColor(0,1,0);

  this->EndPoint2Property = vtkProperty::New();
  this->EndPoint2Property->SetColor(1,1,1);

  this->SelectedEndPoint2Property = vtkProperty::New();
  this->SelectedEndPoint2Property->SetColor(0,1,0);
  
  // Line properties
  this->LineProperty = vtkProperty::New();
  this->LineProperty->SetAmbient(1.0);
  this->LineProperty->SetAmbientColor(1.0,1.0,1.0);
  this->LineProperty->SetLineWidth(2.0);

  this->SelectedLineProperty = vtkProperty::New();
  this->SelectedLineProperty->SetAmbient(1.0);
  this->SelectedLineProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedLineProperty->SetLineWidth(2.0);
}

void vtkFourFlowPlaneRepresentation::SizeHandles() {
  double radius = this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5,this->Sphere->GetCenter());

  this->ConeSource->SetHeight(3.5*radius);
  this->ConeSource->SetRadius(3.0*radius);
  this->ConeSource2->SetHeight(3.5*radius);
  this->ConeSource2->SetRadius(3.0*radius);
  this->Corner1->SetRadius(radius);
  this->Corner2->SetRadius(radius);
  this->Corner3->SetRadius(radius);
  this->Corner4->SetRadius(radius);
  
  this->Sphere->SetRadius(2.0*radius);
}

void vtkFourFlowPlaneRepresentation::BuildRepresentation() {
	if ( ! this->Renderer ) {
		return;
	}

	double *origin = this->PlaneSource->GetCenter();
	double *normal = this->PlaneSource->GetNormal();
	double p2[3];

	// Setup the plane normal
	//double d = this->Outline->GetOutput()->GetLength();
	double d = 1.0;

	p2[0] = origin[0] + 0.10 * d * normal[0];
	p2[1] = origin[1] + 0.10 * d * normal[1];
	p2[2] = origin[2] + 0.10 * d * normal[2];

	this->LineSource1->SetPoint1(origin);
	this->LineSource1->SetPoint2(p2);
	this->ConeSource->SetCenter(p2);
	this->ConeSource->SetDirection(-normal[0], -normal[1], -normal[2]);

	p2[0] = origin[0] - 0.10 * d * normal[0];
	p2[1] = origin[1] - 0.10 * d * normal[1];
	p2[2] = origin[2] - 0.10 * d * normal[2];

	this->LineSource2->SetPoint1(origin[0],origin[1],origin[2]);
	this->LineSource2->SetPoint2(p2);
	this->ConeSource2->SetCenter(p2);
	this->ConeSource2->SetDirection(-normal[0],-normal[1],-normal[2]);

	// Set up the position handle
	this->Sphere->SetCenter(origin[0],origin[1],origin[2]);

	this->SizeHandles();
	PositionHandles();
}

//----------------------------------------------------------------------------
void vtkFourFlowPlaneRepresentation::HighlightPoint(int ptId, int highlight) {
  /*if ( ptId == 0 )
    {
    if ( highlight )
      {
      this->Handle[0]->SetProperty(this->SelectedEndPointProperty);
      this->Point1Representation->SetSelectedProperty(this->SelectedEndPointProperty);
      }
    else
      {
      this->Handle[0]->SetProperty(this->EndPointProperty);
      this->Point1Representation->SetProperty(this->EndPointProperty);
      }
    }
  else if ( ptId == 1 )
    {
    if ( highlight )
      {
      this->Handle[1]->SetProperty(this->SelectedEndPoint2Property);
      this->Point2Representation->SetSelectedProperty(this->SelectedEndPoint2Property);
      }
    else
      {
      this->Handle[1]->SetProperty(this->EndPoint2Property);
      this->Point2Representation->SetProperty(this->EndPoint2Property);
      }
    }
  else //if ( ptId == 2 )
    {
    if ( highlight )
      {
      this->LineHandleRepresentation->SetSelectedProperty(this->SelectedEndPointProperty);
      }
    else
      {
      this->LineHandleRepresentation->SetProperty(this->EndPointProperty);
      }
    }*/
}

//----------------------------------------------------------------------------
void vtkFourFlowPlaneRepresentation::HighlightLine(int highlight) {
  /*if ( highlight )
    {
    this->LineActor->SetProperty(this->SelectedLineProperty);
    }
  else
    {
    this->LineActor->SetProperty(this->LineProperty);
    }*/
}

//----------------------------------------------------------------------------
void vtkFourFlowPlaneRepresentation::ClampPosition(double x[3]) {
  for (int i=0; i<3; i++)
    {
    if ( x[i] < this->InitialBounds[2*i] )
      {
      x[i] = this->InitialBounds[2*i];
      }
    if ( x[i] > this->InitialBounds[2*i+1] )
      {
      x[i] = this->InitialBounds[2*i+1];
      }
    }
}

//----------------------------------------------------------------------------
int vtkFourFlowPlaneRepresentation::InBounds(double x[3])
{
  for (int i=0; i<3; i++)
    {
    if ( x[i] < this->InitialBounds[2*i] ||
         x[i] > this->InitialBounds[2*i+1] )
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------
void vtkFourFlowPlaneRepresentation::GetActors(vtkPropCollection *pc) {
	this->CornerActor1->GetActors(pc);
	this->CornerActor2->GetActors(pc);
	this->CornerActor3->GetActors(pc);
	this->CornerActor4->GetActors(pc);
	this->ConeActor->GetActors(pc);
	this->ConeActor2->GetActors(pc);
	this->LineActor1->GetActors(pc);
	this->LineActor2->GetActors(pc);
	this->SphereActor->GetActors(pc);
}

//----------------------------------------------------------------------------
void vtkFourFlowPlaneRepresentation::ReleaseGraphicsResources(vtkWindow *w) {
	this->LineActor->ReleaseGraphicsResources(w);
	this->Handle[0]->ReleaseGraphicsResources(w);
	this->Handle[1]->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int vtkFourFlowPlaneRepresentation::RenderOpaqueGeometry(vtkViewport *v) {
	int count=0;
	this->BuildRepresentation();
	count += this->CornerActor1->RenderOpaqueGeometry(v);
	count += this->CornerActor2->RenderOpaqueGeometry(v);
	count += this->CornerActor3->RenderOpaqueGeometry(v);
	count += this->CornerActor4->RenderOpaqueGeometry(v);
	count += this->ConeActor->RenderOpaqueGeometry(v);
	count += this->ConeActor2->RenderOpaqueGeometry(v);
	count += this->LineActor1->RenderOpaqueGeometry(v);
	count += this->LineActor2->RenderOpaqueGeometry(v);
	count += this->SphereActor->RenderOpaqueGeometry(v);

	return count;
}

//----------------------------------------------------------------------------
unsigned long vtkFourFlowPlaneRepresentation::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long mTime2=this->Point1Representation->GetMTime();
  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  mTime2=this->Point2Representation->GetMTime();
  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  mTime2=this->LineHandleRepresentation->GetMTime();
  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  
  return mTime;
}

//----------------------------------------------------------------------------
void vtkFourFlowPlaneRepresentation::PrintSelf(ostream& os, vtkIndent indent) {
  this->Superclass::PrintSelf(os,indent);
}



