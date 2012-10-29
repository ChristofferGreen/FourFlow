#include "vtkFourFlowVolumeTrack.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkContourValues.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkSphere.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearSynchronizedTemplates.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkContourFilter.h"
#include "vtkSmartPointer.h"

#include <math.h>

vtkStandardNewMacro(vtkFourFlowVolumeTrack);
vtkCxxSetObjectMacro(vtkFourFlowVolumeTrack,CutFunction,vtkImplicitFunction);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; initial value of 0.0; and
// generating cut scalars turned off.
vtkFourFlowVolumeTrack::vtkFourFlowVolumeTrack(vtkImplicitFunction *cf)
{
  this->CutFunction = cf;

  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_RANGES(), 1);
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_BOUNDS(), 1);
  this->tmpData = vtkStructuredGrid::SafeDownCast(vtkStructuredGrid::New());
  this->contourFilter = vtkContourFilter::New();
  this->resultArray = vtkDataArray::SafeDownCast(vtkAbstractArray::CreateArray(VTK_DOUBLE));
  this->CalculateNormals = true;
}

//----------------------------------------------------------------------------
vtkFourFlowVolumeTrack::~vtkFourFlowVolumeTrack()
{
  this->SetCutFunction(NULL);
  this->tmpData->Delete();
  this->contourFilter->Delete();
  this->resultArray->Delete();
}

//----------------------------------------------------------------------------
unsigned long vtkFourFlowVolumeTrack::GetMTime()
{
	if ( this->CutFunction != NULL )
		return this->CutFunction->GetMTime();
	else this->Superclass::GetMTime();
}


//----------------------------------------------------------------------------
// Cut through data generating surface.
//
int vtkFourFlowVolumeTrack::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
/*vtkFourFlowVolumeTrack : vtkPolyDataAlgorithm
	input: vtkStructuredGrid
	output: vtkPolyData*/

	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	vtkInformation *outInfo = outputVector->GetInformationObject(0);

	// get the input and output
	vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
	vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
	
	vtkDataSet *dsInput = vtkDataSet::SafeDownCast(input);
	vtkDataSet *dsOutput = vtkDataSet::SafeDownCast(output);

	vtkDataSetAttributes* inFD = 0;
	vtkDataSetAttributes* outFD = 0;
	vtkIdType numTuples = 0;
	int attributeDataType = 0; // 0 for point data, 1 for cell data
	inFD = dsInput->GetPointData();
	outFD = dsOutput->GetPointData();
	vtkPointSet* psInput = vtkPointSet::SafeDownCast(input);
	vtkPointSet* psOutput = vtkPointSet::SafeDownCast(output);
	numTuples = dsInput->GetNumberOfPoints();


	if (numTuples < 1) {
		vtkDebugMacro("Empty data.");
		return 1;
	}
	if(!inFD->HasArray("Psi1") || !inFD->HasArray("Psi2") || !inFD->HasArray("Psi3"))
		return 0;
	vtkFloatArray *Psi1 = vtkFloatArray::SafeDownCast(inFD->GetAbstractArray("Psi1"));
	vtkFloatArray *Psi2 = vtkFloatArray::SafeDownCast(inFD->GetAbstractArray("Psi2"));
	vtkFloatArray *Psi3 = vtkFloatArray::SafeDownCast(inFD->GetAbstractArray("Psi3"));
    resultArray->SetNumberOfComponents(1);
	resultArray->SetNumberOfTuples(Psi1->GetNumberOfTuples());
	int tuples = Psi1->GetNumberOfTuples();
	if(this->CutFunction && this->CutFunction->IsA("vtkPlane")) {
		vtkPlane *plane = vtkPlane::SafeDownCast(this->CutFunction);
		double normal[3];
		double origin[3];
		plane->GetNormal(&normal[0]);
		plane->GetOrigin(&origin[0]);
		for(int i = 0; i < tuples; i++) {
			float result =	normal[0]*(Psi1->GetComponent(i, 0)-origin[0])+
							normal[1]*(Psi2->GetComponent(i, 0)-origin[1])+
							normal[2]*(Psi3->GetComponent(i, 0)-origin[2]);
			resultArray->SetTuple(i, &result);
		}
	}
	else if(this->CutFunction && this->CutFunction->IsA("vtkSphere")) {
		vtkSphere *sphere = vtkSphere::SafeDownCast(this->CutFunction);
		double center[3];
		double radius;
		radius = sphere->GetRadius();
		sphere->GetCenter(&center[0]);
		for(int i = 0; i < tuples; i++) {
			float result =	((Psi1->GetComponent(i, 0)-center[0])*(Psi1->GetComponent(i, 0)-center[0])+
							(Psi2->GetComponent(i, 0)-center[1])*(Psi2->GetComponent(i, 0)-center[1])+
							(Psi3->GetComponent(i, 0)-center[2])*(Psi3->GetComponent(i, 0)-center[2]));
			resultArray->SetTuple(i, &result);
		}
	}
	resultArray->SetName("Result");

	tmpData->ShallowCopy(input);
	tmpData->GetPointData()->AddArray(resultArray);
	
	contourFilter->SetInput(tmpData);
	if(this->CalculateNormals)
		contourFilter->SetComputeNormals(true);
	else
		contourFilter->SetComputeNormals(false);
	contourFilter->SetValue(0, GetISO());
	contourFilter->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS, "Result");
	contourFilter->UseScalarTreeOn();
	contourFilter->Update();
	this->GetOutput()->ShallowCopy(contourFilter->GetOutput());

	return 1;
}

void vtkFourFlowVolumeTrack::SetCalculateNormals(int mode) {
	this->CalculateNormals = mode;
	this->Modified();
}

float vtkFourFlowVolumeTrack::GetISO() {
	if(this->CutFunction && this->CutFunction->IsA("vtkPlane")) {
		return 0.0;
	}
	else if(this->CutFunction && this->CutFunction->IsA("vtkSphere")) {
		vtkSphere *sphere = vtkSphere::SafeDownCast(this->CutFunction);
		double radius;
		radius = sphere->GetRadius();
		return radius*radius;
	}
	return 0.1;
}

void vtkFourFlowVolumeTrack::SetISO(float iso) {
}


//----------------------------------------------------------------------------
int vtkFourFlowVolumeTrack::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkFourFlowVolumeTrack::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
