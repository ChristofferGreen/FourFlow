/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMantaView VTK level view that uses Manta instead of GL
// .SECTION Description
// A 3D view that uses the manta ray tracer instead of openGL for rendering

#ifndef __vtkPVVolumeSliceView_h
#define __vtkPVVolumeSliceView_h

#include "vtkPVRenderView.h"

class vtkDataRepresentation;

class vtkPVVolumeSliceView : public vtkPVView
{
public:
  static vtkPVVolumeSliceView* New();
  vtkTypeMacro(vtkPVVolumeSliceView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent);
  virtual void StillRender(void) {}
  virtual void InteractiveRender(void) {}

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // @CallOnAllProcessess
  virtual void Initialize(unsigned int id);

//BTX
protected:
  vtkPVVolumeSliceView();
  ~vtkPVVolumeSliceView();

private:
  vtkPVVolumeSliceView(const vtkPVVolumeSliceView&); // Not implemented
  void operator=(const vtkPVVolumeSliceView&); // Not implemented
//ETX
};

#endif
