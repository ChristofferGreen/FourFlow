/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFourFlowIsoSurfaceFilter - generate isosurfaces/isolines from scalar values
// .SECTION Description
// vtkFourFlowIsoSurfaceFilter is an extension to vtkContourFilter. It adds the
// ability to generate isosurfaces / isolines for AMR dataset.
//
// .SECTION Caveats
// Certain flags in vtkAMRDualContour are assumed to be ON.
//
// .SECTION See Also
// vtkContourFilter vtkAMRDualContour

#ifndef __vtkFourFlowIsoSurfaceCleanupFilter_h
#define __vtkFourFlowIsoSurfaceCleanupFilter_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkContourFilter.h"
#include <iostream>

class vtkFourFlowIsoSurfaceCleanupFilter : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkFourFlowIsoSurfaceCleanupFilter, vtkContourFilter);

  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkFourFlowIsoSurfaceCleanupFilter* New();

  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);
  float isoValue;
  void SetIsoValue(float val) {this->isoValue=val;std::cout<<"set iso val: "<<val<<std::endl;this->SetValue(0,val);}
  float GetIsoValue() {return this->isoValue;std::cout<<"get iso val: "<<this->isoValue<<std::endl;}

  int cleanupValue;
  void SetCleanupValue(int val) {this->cleanupValue=val;std::cout<<"set cleanup val: "<<val<<std::endl;this->Modified();}
  float GetCleanupValue() {std::cout<<"get cleanup val: "<<this->isoValue<<std::endl;return this->cleanupValue;}

  int useCleanupValue;
  void SetUseCleanup(int val) {this->useCleanupValue=val;std::cout<<"set use cleanup val: "<<val<<std::endl;this->Modified();}
  float GetUseCleanup() {std::cout<<"get use cleanup val: "<<this->useCleanupValue<<std::endl;return this->useCleanupValue;}


protected:

  vtkFourFlowIsoSurfaceCleanupFilter();
 ~vtkFourFlowIsoSurfaceCleanupFilter();

 virtual int RequestData(vtkInformation* request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector);

 virtual int RequestDataObject(vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector);

 virtual int FillInputPortInformation(int port, vtkInformation* info);
 virtual int FillOutputPortInformation(int port, vtkInformation* info);

 // Description:
 // Class superclass request data. Also handles iterating over
 // vtkHierarchicalBoxDataSet.
 int ContourUsingSuperclass(
   vtkInformation* request, vtkInformationVector** inputVector,
   vtkInformationVector* outputVector);

private:
 vtkFourFlowIsoSurfaceCleanupFilter(const vtkFourFlowIsoSurfaceCleanupFilter&); // Not implemented.
 void operator=(const vtkFourFlowIsoSurfaceCleanupFilter&);     // Not implemented.
};

#endif // __vtkFourFlowIsoSurfaceFilter_h
