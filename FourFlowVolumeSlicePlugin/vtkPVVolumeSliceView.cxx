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
#include "vtkPVVolumeSliceView.h"

#include "vtkCamera.h"
#include "vtkDataRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkRenderViewBase.h"

vtkStandardNewMacro(vtkPVVolumeSliceView);
//----------------------------------------------------------------------------
vtkPVVolumeSliceView::vtkPVVolumeSliceView()
{
}

//----------------------------------------------------------------------------
vtkPVVolumeSliceView::~vtkPVVolumeSliceView()
{
}

//----------------------------------------------------------------------------
void vtkPVVolumeSliceView::Initialize(unsigned int id)
{
  this->Superclass::Initialize(id);
}

//----------------------------------------------------------------------------
void vtkPVVolumeSliceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
