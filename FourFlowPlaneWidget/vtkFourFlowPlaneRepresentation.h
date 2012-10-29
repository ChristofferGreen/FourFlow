#ifndef __vtkFourFlowPlaneRepresentation_h
#define __vtkFourFlowPlaneRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkPolyDataMapper;
class vtkLineSource;
class vtkSphereSource;
class vtkProperty;
class vtkPolyData;
class vtkPolyDataAlgorithm;
class vtkPointHandleRepresentation3D;
class vtkBox;
class vtkFollower;
class vtkVectorText;
class vtkPolyDataMapper;
class vtkCellPicker;
class vtkConeSource;
class vtkPlaneSource;
class vtkTransform;

class vtkFourFlowPlaneRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkFourFlowPlaneRepresentation *New();

  // Description:
  // Standard methods for the class.
  vtkTypeMacro(vtkFourFlowPlaneRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods to Set/Get the coordinates of the two points defining
  // this representation. Note that methods are available for both
  // display and world coordinates.
  //---------------------------------------------------------------------
  void SetOrigin(double pos[3]);
  void SetPoint1(double pos[3]);
  void SetPoint2(double pos[3]);
  double *GetOrigin();
  double *GetPoint1();
  double *GetPoint2();
  virtual void MovePoint1(double *p1, double *p2);
  void Rotate(double X, double Y, double *p1, double *p2, double *vpn);
  void TranslatePlane(double *p1, double *p2);
  void Push(double *p1, double *p2);
  //---------------------------------------------------------------------
  
  // Description:
  // This method is used to specify the type of handle representation to
  // use for the three internal vtkHandleWidgets within vtkLineWidget2.
  // To use this method, create a dummy vtkHandleWidget (or subclass),
  // and then invoke this method with this dummy. Then the 
  // vtkLineRepresentation uses this dummy to clone three vtkHandleWidgets
  // of the same type. Make sure you set the handle representation before
  // the widget is enabled. (The method InstantiateHandleRepresentation()
  // is invoked by the vtkLineWidget2.)
  void SetHandleRepresentation(vtkPointHandleRepresentation3D *handle);
  void InstantiateHandleRepresentation();

  // Description:
  // Get the three handle representations used for the vtkLineWidget2. 
  vtkGetObjectMacro(Point1Representation,vtkPointHandleRepresentation3D);
  vtkGetObjectMacro(Point2Representation,vtkPointHandleRepresentation3D);
  vtkGetObjectMacro(LineHandleRepresentation,vtkPointHandleRepresentation3D);

  // Description:
  // Get the end-point (sphere) properties. The properties of the end-points 
  // when selected and unselected can be manipulated.
  vtkGetObjectMacro(EndPointProperty,vtkProperty);
  vtkGetObjectMacro(SelectedEndPointProperty,vtkProperty);

  // Description:
  // Get the end-point (sphere) properties. The properties of the end-points 
  // when selected and unselected can be manipulated.
  vtkGetObjectMacro(EndPoint2Property,vtkProperty);
  vtkGetObjectMacro(SelectedEndPoint2Property,vtkProperty);
  
  // Description:
  // Get the line properties. The properties of the line when selected
  // and unselected can be manipulated.
  vtkGetObjectMacro(LineProperty,vtkProperty);
  vtkGetObjectMacro(SelectedLineProperty,vtkProperty);

  // Description:
  // The tolerance representing the distance to the widget (in pixels) in
  // which the cursor is considered near enough to the line or end point 
  // to be active.
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void PlaceWidget(double bounds[6]);
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual double *GetBounds();
  
  // Description:
  // Methods supporting the rendering process.
  virtual void GetActors(vtkPropCollection *pc);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  
//BTX - manage the state of the widget
  enum {Outside=0,Moving,Rotating,Pushing,MovingOrigin,MovingPlane,MovingCorner1,MovingCorner2,MovingCorner3,MovingCorner4,OnP1,OnP2,TranslatingP1,TranslatingP2,OnLine,Scaling};
//ETX

  // Description:
  // The interaction state may be set from a widget (e.g., vtkLineWidget2) or
  // other object. This controls how the interaction with the widget
  // proceeds. Normally this method is used as part of a handshaking
  // process with the widget: First ComputeInteractionState() is invoked that
  // returns a state based on geometric considerations (i.e., cursor near a
  // widget feature), then based on events, the widget may modify this
  // further.
  vtkSetClampMacro(InteractionState,int,Outside,Scaling);

  // Description:
  // Sets the visual appearance of the representation based on the
  // state it is in. This state is usually the same as InteractionState.
  virtual void SetRepresentationState(int);
  vtkGetMacro(RepresentationState, int);

  // Description:
  // Overload the superclasses' GetMTime() because internal classes
  // are used to keep the state of the representation.
  virtual unsigned long GetMTime();

  // Description:
  // Overridden to set the rendererer on the internal representations.
  virtual void SetRenderer(vtkRenderer *ren);

protected:
  vtkFourFlowPlaneRepresentation();
  ~vtkFourFlowPlaneRepresentation();
  
  //-----------------------------------------------------------------------
  vtkPlaneSource    *PlaneSource;
  vtkSphereSource   *Corner1;
  vtkActor          *CornerActor1;
  vtkPolyDataMapper *CornerMapper1;
  vtkSphereSource   *Corner2;
  vtkActor          *CornerActor2;
  vtkPolyDataMapper *CornerMapper2;
  vtkSphereSource   *Corner3;
  vtkActor          *CornerActor3;
  vtkPolyDataMapper *CornerMapper3;
  vtkSphereSource   *Corner4;
  vtkActor          *CornerActor4;
  vtkPolyDataMapper *CornerMapper4;
  // The + normal cone
  vtkConeSource     *ConeSource;
  vtkPolyDataMapper *ConeMapper;
  vtkActor          *ConeActor;
  // The + normal line
  vtkLineSource     *LineSource1;
  vtkPolyDataMapper *LineMapper1;
  vtkActor          *LineActor1;
  // The - normal cone
  vtkConeSource     *ConeSource2;
  vtkPolyDataMapper *ConeMapper2;
  vtkActor          *ConeActor2;
  // The - normal line
  vtkLineSource     *LineSource2;
  vtkPolyDataMapper *LineMapper2;
  vtkActor          *LineActor2;
  // The origin positioning handle
  vtkSphereSource   *Sphere;
  vtkPolyDataMapper *SphereMapper;
  vtkActor          *SphereActor;

  vtkCellPicker *Picker;
  vtkTransform *Transform;
  
  void PositionHandles();
  //-----------------------------------------------------------------------

  // The handle and the rep used to close the handles
  vtkPointHandleRepresentation3D *HandleRepresentation;
  vtkPointHandleRepresentation3D *Point1Representation;
  vtkPointHandleRepresentation3D *Point2Representation;
  vtkPointHandleRepresentation3D *LineHandleRepresentation;

  // Manage how the representation appears
  int RepresentationState;

  // the line
  vtkActor          *LineActor;
  vtkPolyDataMapper *LineMapper;
  vtkLineSource     *LineSource;

  // glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkPolyDataMapper **HandleMapper;
  vtkSphereSource   **HandleGeometry;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *EndPointProperty;
  vtkProperty *SelectedEndPointProperty;
  vtkProperty *EndPoint2Property;
  vtkProperty *SelectedEndPoint2Property;
  vtkProperty *LineProperty;
  vtkProperty *SelectedLineProperty;
  void         CreateDefaultProperties();

  // Selection tolerance for the handles and the line
  int Tolerance;

  // Helper members
  int  ClampToBounds;
  void ClampPosition(double x[3]);
  void HighlightPoint(int ptId, int highlight);
  void HighlightLine(int highlight);
  int  InBounds(double x[3]);
  void SizeHandles();

  // Ivars used during widget interaction to hold initial positions
  double StartP1[3];
  double StartP2[3];
  double StartLineHandle[3];
  double Length;
  double LastEventPosition[3];

  // Support GetBounds() method
  vtkBox *BoundingBox;
  
  // Need to keep track if we have successfully initialized the display position. 
  // The widget tends to do stuff in world coordinates, put if the renderer has
  // not been assigned, then certain operations do not properly update the display
  // position.
  int InitializedDisplayPosition;

private:
  vtkFourFlowPlaneRepresentation(const vtkFourFlowPlaneRepresentation&);  //Not implemented
  void operator=(const vtkFourFlowPlaneRepresentation&);  //Not implemented
};

#endif
