#ifndef __vtkFourFlowVolumeTrack_h
#define __vtkFourFlowVolumeTrack_h

#include "vtkPolyDataAlgorithm.h"

class vtkImplicitFunction;
class vtkStructuredGrid;
class vtkContourFilter;
class vtkDataArray;

class vtkFourFlowVolumeTrack : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkFourFlowVolumeTrack,vtkPolyDataAlgorithm);

  // Description:
  // Construct with user-specified implicit function; initial value of 0.0;
  static vtkFourFlowVolumeTrack *New();

  // Description:
  // Override GetMTime because we delegate to the cut widget
  unsigned long GetMTime();

  // Description
  // Specify the implicit function to perform the cutting.
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);

  float GetISO();
  void SetISO(float iso);

  void SetCalculateNormals(int mode);
  vtkGetMacro(CalculateNormals, int);


protected:
  vtkFourFlowVolumeTrack(vtkImplicitFunction *cf=NULL);
  ~vtkFourFlowVolumeTrack();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  vtkImplicitFunction *CutFunction;
private:
  vtkFourFlowVolumeTrack(const vtkFourFlowVolumeTrack&);  // Not implemented.
  void operator=(const vtkFourFlowVolumeTrack&);  // Not implemented.

  vtkStructuredGrid *tmpData;
  vtkContourFilter *contourFilter;
  vtkDataArray *resultArray;
  int CalculateNormals;
};

#endif
