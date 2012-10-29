/*=========================================================================

  Program:   ParaView
  Module:    vtkFourFlowSAX.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFourFlowSAX.h"

#include "vtkExtentTranslator.h"
#include "vtkExtractGrid.h"
#include "vtkExtractVOI.h"
#include "vtkExtractRectilinearGrid.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkMultiBlockDataSet.h"

#include <string>
using namespace std;

vtkStandardNewMacro(vtkFourFlowSAX);

//----------------------------------------------------------------------------
// Construct object to extract all of the input data.
vtkFourFlowSAX::vtkFourFlowSAX()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_LARGE_INTEGER;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
  
	this->PlaneRange[0] = 0;
	this->PlaneRange[1] = 255;
  this->IncludeBoundary = 0;
  this->hasRequestedData = false;

  this->ExtractGrid = vtkExtractGrid::New();
  this->ExtractVOI  = vtkExtractVOI::New();
  this->ExtractRG   = vtkExtractRectilinearGrid::New();
}

//----------------------------------------------------------------------------
vtkFourFlowSAX::~vtkFourFlowSAX()
{
  if(this->ExtractVOI)
    {
    this->ExtractVOI->Delete();
    }
  if(this->ExtractGrid)
    {
    this->ExtractGrid->Delete();
    }
  if(this->ExtractRG)
    {
    this->ExtractRG->Delete();
    }
}

void vtkFourFlowSAX::SetSAXPlane(int plane) {
	//if(this->hasRequestedData) {
		this->VOI[4] = plane;
		this->VOI[5] = plane;
	//}
	this->SAXPlane = plane;
	this->Modified();
}

int vtkFourFlowSAX::GetSAXPlane() {
	return this->SAXPlane;
}


//----------------------------------------------------------------------------
template <class FilterType>
void vtkFourFlowSAXProcessRequest(
  FilterType* filter, vtkFourFlowSAX* self,
  vtkInformation*request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector  )
{
  filter->SetVOI(self->GetVOI());
  filter->SetSampleRate(self->GetSampleRate());
  filter->ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkFourFlowSAX::RequestUpdateExtent(vtkInformation* request, 
                                         vtkInformationVector** inputVector, 
                                         vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (output->GetDataObjectType() == VTK_IMAGE_DATA)
    {
    vtkFourFlowSAXProcessRequest(
      this->ExtractVOI, 
      this,
      request,
      inputVector,
      outputVector);
    }
  else if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    vtkFourFlowSAXProcessRequest(
      this->ExtractGrid, 
      this,
      request,
      inputVector,
      outputVector);
    }
  else if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    vtkFourFlowSAXProcessRequest(
      this->ExtractRG, 
      this,
      request,
      inputVector,
      outputVector);
    }


  // We can handle anything.
  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);


  return 1;
}

//----------------------------------------------------------------------------
int vtkFourFlowSAX::RequestInformation(vtkInformation* request, 
                                        vtkInformationVector** inputVector, 
                                        vtkInformationVector* outputVector)
{
	vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
	vtkDataObject* inputDataObject = inInfo->Get(vtkDataObject::DATA_OBJECT());
	if(string(inputDataObject->GetClassName()).compare("vtkMultiBlockDataSet") == 0) {
		vtkMultiBlockDataSet *multiDataSet = vtkMultiBlockDataSet::SafeDownCast(inputDataObject);
		vtkDataObject *dataObject = multiDataSet->GetBlock(0);
		vtkStructuredGrid *structuredGrid = vtkStructuredGrid::SafeDownCast(dataObject);
		int dimensions[55];
		if(!structuredGrid)
			return 1;
		structuredGrid->GetDimensions(&dimensions[0]);
		this->PlaneRange[1] = dimensions[2]-1;
		this->Modified();
	}


  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (output->GetDataObjectType() == VTK_IMAGE_DATA)
    {
    vtkFourFlowSAXProcessRequest(
      this->ExtractVOI, 
      this,
      request,
      inputVector,
      outputVector);
    }
  else if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    this->ExtractGrid->SetIncludeBoundary(this->IncludeBoundary);
    vtkFourFlowSAXProcessRequest(
      this->ExtractGrid, 
      this,
      request,
      inputVector,
      outputVector);
    }
  else if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    this->ExtractRG->SetIncludeBoundary(this->IncludeBoundary);
    vtkFourFlowSAXProcessRequest(
      this->ExtractRG, 
      this,
      request,
      inputVector,
      outputVector);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkFourFlowSAX::RequestData(vtkInformation* request, 
                                 vtkInformationVector** inputVector, 
                                 vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = 
    vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (output->GetDataObjectType() == VTK_IMAGE_DATA)
    {
    vtkFourFlowSAXProcessRequest(
      this->ExtractVOI, 
      this,
      request,
      inputVector,
      outputVector);
    }
  else if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    this->ExtractGrid->SetIncludeBoundary(this->IncludeBoundary);
    vtkFourFlowSAXProcessRequest(
      this->ExtractGrid, 
      this,
      request,
      inputVector,
      outputVector);
    }
  else if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    this->ExtractRG->SetIncludeBoundary(this->IncludeBoundary);
    vtkFourFlowSAXProcessRequest(
      this->ExtractRG, 
      this,
      request,
      inputVector,
      outputVector);
    }
	/*vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	if(this->first && inInfo->Get(vtkDataObject::DATA_OBJECT())->GetDataObjectType() == VTK_STRUCTURED_GRID) {
		vtkStructuredGrid *grid = vtkStructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
		if(grid) {
			int voi[19];
			grid->GetDimensions(voi);
			this->PlaneRange[1] = voi[2]-1;
			cout << "dimensions: " << voi[0] << " " << voi[1] << " " << voi[2] << endl;
			this->Modified();
		}
	}
	this->hasRequestedData = true;*/

  return 1;
}

//----------------------------------------------------------------------------
void vtkFourFlowSAX::SetSampleRateI(int ratei)
{
  if (this->SampleRate[0] == ratei)
    {
    return;
    }
  
  this->SampleRate[0] = ratei;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFourFlowSAX::SetSampleRateJ(int ratej)
{
  if (this->SampleRate[1] == ratej)
    {
    return;
    }
  
  this->SampleRate[1] = ratej;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFourFlowSAX::SetSampleRateK(int ratek)
{
  if (this->SampleRate[2] == ratek)
    {
    return;
    }
  
  this->SampleRate[2] = ratek;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkFourFlowSAX::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->ExtractVOI, "ExtractVOI");
  vtkGarbageCollectorReport(collector, this->ExtractGrid, "ExtractGrid");
  vtkGarbageCollectorReport(collector, this->ExtractRG, "ExtractRG");
}

//----------------------------------------------------------------------------
void vtkFourFlowSAX::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VOI: \n";
  os << indent << "  Imin,Imax: (" << this->VOI[0] << ", " 
     << this->VOI[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" << this->VOI[2] << ", " 
     << this->VOI[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" << this->VOI[4] << ", " 
     << this->VOI[5] << ")\n";

  os << indent << "Sample Rate: (" << this->SampleRate[0] << ", "
               << this->SampleRate[1] << ", "
               << this->SampleRate[2] << ")\n";

  os << indent << "Include Boundary: " 
     << (this->IncludeBoundary ? "On\n" : "Off\n");
}
