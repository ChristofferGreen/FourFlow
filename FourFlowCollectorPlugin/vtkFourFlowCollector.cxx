#include "vtkFourFlowCollector.h"
#include "vtkTemporalStreamTracer.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkSmartPointer.h"
#include "vtkTemporalInterpolatedVelocityField.h"
#include "vtkTemporalDataSet.h"
#include "vtkOutputWindow.h"
#include "vtkAbstractParticleWriter.h"
#include "vtkToolkits.h" // For VTK_USE_MPI 
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkPolygon.h"
#include "vtkTriangleFilter.h"
#include "vtkMassProperties.h"
#include "vtkCellLocator.h"
#include <sstream>
#include <set>

using namespace std;

vtkStandardNewMacro(vtkFourFlowCollector);

vtkFourFlowCollector::vtkFourFlowCollector() : vtkPolyDataAlgorithm() {
	cout << "vtkFourFlowCollector::vtkFourFlowCollector()" << endl;
	this->SetNumberOfInputPorts(2);
	this->SetNumberOfOutputPorts(1);
	this->CurrentTimeIndex = 0;
	this->continueExecuting = true;
}

vtkFourFlowCollector::~vtkFourFlowCollector() {
	cout << "vtkFourFlowCollector::~vtkFourFlowCollector()" << endl;
}

int vtkFourFlowCollector::FillInputPortInformation(int port, vtkInformation* info) {
	cout << "vtkFourFlowCollector::FillInputPortInformation" << endl;
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");    
	return 1;
}

vector<Point> vtkFourFlowCollector::getPoints(vtkPolyData *polyData) {
	vector<Point> points;
	cout << "particle?: " << polyData->GetClassName() << " " << polyData->GetNumberOfPoints() << endl;
	if(vtkDataArray *pointIds = polyData->GetPointData()->GetArray("ParticleId")) {
		cout << pointIds << " " << pointIds->GetClassName() << " " << pointIds->GetNumberOfTuples() <<endl;
		vtkIntArray *pointIdsInt = (vtkIntArray*)pointIds;
		for(int i = 0; i < pointIdsInt->GetNumberOfTuples(); i++) {
			Point point;
			point.id = pointIdsInt->GetValue(i);
			polyData->GetPoint(i, point.pos);
			points.push_back(point);
		}
	}
	return points;
}

LineCollection vtkFourFlowCollector::getCurrentLines(int currentFrame, vtkPolyData *particlePolyData) {
	if(currentFrame == 0) {
		this->pointBuffer.previousPoints = getPoints(particlePolyData);
		this->pointBuffer.currentPoints = this->pointBuffer.previousPoints;
	}
	else {
		this->pointBuffer.previousPoints = this->pointBuffer.currentPoints;
		this->pointBuffer.currentPoints = getPoints(particlePolyData);
	}
	LineCollection lines;
	for(int i = 0; i < this->pointBuffer.previousPoints.size(); i++) {
		for(int j = 0; j < this->pointBuffer.currentPoints.size(); j++) {
			if(this->pointBuffer.previousPoints[i].id == this->pointBuffer.currentPoints[j].id) {
				Line line;
				line.p1 = this->pointBuffer.previousPoints[i];
				line.p2 = this->pointBuffer.currentPoints[j];
				lines.lines.push_back(line);
			}
		}
	}
	return lines;
}

LineCollection vtkFourFlowCollector::getCollidedLines(LineCollection allLines, vtkPolyData *ringPolygonData) {
	LineCollection coolidedLines;
	if(vtkCell *polygon = ringPolygonData->GetCell(0)) {
		for(int i = 0; i < allLines.lines.size(); i++) {
			double t=0.0;
			double x[3];
			double pcoords[3]={0.0};
			int subid=0;
			if(polygon->IntersectWithLine(allLines.lines[i].p1.pos, allLines.lines[i].p2.pos, 1E-6, t, x, pcoords, subid)) {
				coolidedLines.lines.push_back(allLines.lines[i]);
			}
		}
	}
	return coolidedLines;
}

int vtkFourFlowCollector::getIntersectedPointsThisFrame(vtkPolyData *particlePolyData, vtkPolyData *ringPolygonData, int currentFrame) {
	LineCollection allLines = this->getCurrentLines(currentFrame, particlePolyData);
	LineCollection collidedLines = this->getCollidedLines(allLines, ringPolygonData);
	cout << "|||---collided lines: " << collidedLines.lines.size() << endl;
	return collidedLines.lines.size();
}

int vtkFourFlowCollector::RequestData(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector) {
	cout << "vtkFourFlowCollector::RequestData2" << endl;
	/*if(this->continueExecuting == false)
		return 1;*/
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
	vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);
	//vtkTable *table = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
	double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	int size = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

	/*if(this->CurrentTimeIndex == 0) {
	}
	if(this->CurrentTimeIndex < inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS())) {
		if(!table->GetColumnByName("RingPolygon1")) {
			vtkFloatArray *arr = vtkFloatArray::New();
			arr->SetName("RingPolygon1");
			table->AddColumn(arr);
		}
		table->SetNumberOfRows(size);

		vtkPolyData *particlePolyData = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
		vtkPolyData *ringPolygonData = vtkPolyData::SafeDownCast(inInfo2->Get(vtkDataObject::DATA_OBJECT()));
		int points = this->getIntersectedPointsThisFrame(particlePolyData, ringPolygonData, this->CurrentTimeIndex);
		table->SetValue(this->CurrentTimeIndex, 0, points);

		request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
		this->continueExecuting = true;
		this->UpdateProgress(static_cast<double>(this->CurrentTimeIndex)/size);
		this->CurrentTimeIndex++;
	}
	else {
		cout << "stop" << endl;
		request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
		this->continueExecuting = false;
		table->Update();
		this->CurrentTimeIndex = 0;
	}*/

	vtkInformation *outInfo0 = outputVector->GetInformationObject(0);
	vtkPolyData   *output0 = vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));

	vtkPolyData *particlePolyData = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
	LineCollection lines = this->getCurrentLines(this->CurrentTimeIndex, particlePolyData);
	vtkPoints *points = vtkPoints::New();
	points->Allocate(lines.lines.size()*2);

	output0->Allocate(lines.lines.size());
	for(int i = 0; i < lines.lines.size(); i++) {
		Line line = lines.lines[i];
		vtkIdType connectivity[2];
		connectivity[0] = points->InsertNextPoint(line.p1.pos[0], line.p1.pos[1], line.p1.pos[2]);
		connectivity[1] = points->InsertNextPoint(line.p2.pos[0], line.p2.pos[1], line.p2.pos[2]);
		output0->InsertNextCell(VTK_LINE,2,connectivity);
	}
	output0->SetPoints(points);

	return 1;
}

int vtkFourFlowCollector::ProcessRequest(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector) {
	cout << "vtkFourFlowCollector::ProcessRequest" << endl;
	if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
		return this->RequestInformation(request, inputVector, outputVector);       
    }
	if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
      return this->RequestUpdateExtent(request, inputVector, outputVector);
    }
	if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
		return this->RequestData(request, inputVector, outputVector);       
    }
	return 1;
}

int vtkFourFlowCollector::RequestInformation(vtkInformation* request,
                              vtkInformationVector** inputVector,
                              vtkInformationVector* outputVector) {
	cout << "vtkFourFlowCollector::RequestInformation" << endl;
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS())) {
		outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	}
	if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE())) {
		outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
	}


	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) ) {
		this->NumberOfTimeSteps = inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
	}
	else {
		this->NumberOfTimeSteps = 0;
	}

	return 1;
}

int vtkFourFlowCollector::RequestUpdateExtent(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector) {
	cout << "vtkFourFlowCollector::RequestUpdateExtent" << endl;
	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	vtkInformation *inInfo2 = inputVector[1]->GetInformationObject(0);
	double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	if(inTimes) {
		inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), &inTimes[this->CurrentTimeIndex], 1);
		inInfo2->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), &inTimes[this->CurrentTimeIndex], 1);
	}
	return 1;
}

void vtkFourFlowCollector::AddSourceConnection(vtkAlgorithmOutput* input) {
	cout << "vtkFourFlowCollector::AddSourceConnection" << endl;
	this->AddInputConnection(1, input);
}

void vtkFourFlowCollector::RemoveAllSources() {
	cout << "vtkFourFlowCollector::RemoveAllSources" << endl;
	this->SetInputConnection(1, 0);
}