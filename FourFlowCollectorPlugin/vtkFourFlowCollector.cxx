#include "vtkFourFlowCollector.h"
#include "vtkTemporalStreamTracer.h"
#include "vtkAlgorithmOutput.h"
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
#include "vtkTemporalInterpolatedVelocityField.h"
//#include "vtkTemporalDataSet.h"
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

vtkFourFlowCollector::vtkFourFlowCollector() : vtkTableAlgorithm() {
	std::cout << "vtkFourFlowCollector::vtkFourFlowCollector" << std::endl;
	this->SetNumberOfOutputPorts(1);
	this->SetNumberOfInputPorts(2);
	this->CurrentTimeIndex = 0;
	this->continueExecuting = true;
}

vtkFourFlowCollector::~vtkFourFlowCollector() {
	std::cout << "vtkFourFlowCollector::~vtkFourFlowCollector" << std::endl;
}

int vtkFourFlowCollector::FillInputPortInformation(int port, vtkInformation* info) {
	std::cout << "vtkFourFlowCollector::FillInputPortInformation" << std::endl;
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");    
	if(port==1)
		info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
	return 1;
}

vector<Point> vtkFourFlowCollector::getPoints(vtkPolyData *polyData) {
	vector<Point> points;
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
		cout << "polygon verts: " << polygon->GetNumberOfPoints() << endl;
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

string intToString(int i) {
	string res;
	std::stringstream out;
	out << i;
	res = out.str();
	return res;
}

int vtkFourFlowCollector::RequestData(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector) {
	std::cout << "vtkFourFlowCollector::RequestData" << std::endl;
	/*if(this->continueExecuting == false)
		return 1;*/
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
	vtkTable *table = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
	double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	int size = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	int numberOfSources = inputVector[1]->GetNumberOfInformationObjects();
	int numberOfInputs = inputVector[0]->GetNumberOfInformationObjects();
	cout << "numberOfSources: " << numberOfSources << endl; 
	cout << "numberOfInputs: " << numberOfInputs << endl; 

	const int CollisionsColumn = 0;
	string collisionsName = string("Collisions with Polygon Ring")+string(": Made up of ")+intToString(vtkPolyData::SafeDownCast(inputVector[1]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()))->GetNumberOfPoints())+string(" points");
	if(!table->GetColumnByName(collisionsName.c_str())) {
		vtkFloatArray *collisions = vtkFloatArray::New();
		collisions->SetName(collisionsName.c_str());
		table->AddColumn(collisions);
	}

	const int TotalEjectedColumn = 1;
	std::string totalEjectedparticlesName = "Alive (Ejected) particles";
	if(!table->GetColumnByName(totalEjectedparticlesName.c_str())) {
		vtkFloatArray *totalEjectedParticles = vtkFloatArray::New();
		totalEjectedParticles->SetName(totalEjectedparticlesName.c_str());
		table->AddColumn(totalEjectedParticles);
	}

	if(this->CurrentTimeIndex == 0) {
	}
	if(this->CurrentTimeIndex < inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS())) {
		table->SetNumberOfRows(size);
		vtkPolyData *particlePolyData = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
		LineCollection allLines = this->getCurrentLines(this->CurrentTimeIndex, particlePolyData);
		std::cout << "number of lines: " << allLines.lines.size() << std::endl;
		
		int collidedParticles = this->getCollidedLines(allLines, vtkPolyData::SafeDownCast(inputVector[1]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()))).lines.size();
		int ejectedParticles = vtkPolyData::SafeDownCast(inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()))->GetNumberOfPoints();
		table->SetValue(this->CurrentTimeIndex, CollisionsColumn, collidedParticles);
		table->SetValue(this->CurrentTimeIndex, TotalEjectedColumn, ejectedParticles);

		request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
		this->continueExecuting = true;
		double progress = static_cast<double>(this->CurrentTimeIndex)/static_cast<double>(size);
		std::cout << "progress: " << progress << std::endl;
		this->UpdateProgress(progress);
		this->CurrentTimeIndex++;
	}
	else {
		request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
		this->continueExecuting = false;
		//table->Update();
		this->CurrentTimeIndex = 0;

		{
			vtkFloatArray *arr = vtkFloatArray::New();
			double prevValue = 0.0;
			for(int i = 0; i < inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()); i++) {
				double currentValue=0.0;
				currentValue+=table->GetValue(i, CollisionsColumn).ToDouble();
				arr->InsertNextTuple1(prevValue+currentValue);
				prevValue += currentValue;
			}
			arr->SetName(("Total amount of particle collisions: " + intToString(int(prevValue))).c_str());
			table->AddColumn(arr);
		}

		const int TotalCollisions = 2;
		{
			vtkFloatArray *arr = vtkFloatArray::New();
			for(int i = 0; i < inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()); i++) {
				double collisions = table->GetValue(i, TotalCollisions).ToDouble();
				double ejections= table->GetValue(i, TotalEjectedColumn).ToDouble();
				arr->InsertNextTuple1(collisions/ejections);
			}
			arr->SetName("Particles Collided/Particles Alive");
			table->AddColumn(arr);
		}
	}

	return 1;
}

int vtkFourFlowCollector::ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector) {
	std::cout << "vtkFourFlowCollector::ProcessRequest" << std::endl;
	if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION())) 
		return this->RequestInformation(request, inputVector, outputVector);       
	if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
      return this->RequestUpdateExtent(request, inputVector, outputVector);
	if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
		return this->RequestData(request, inputVector, outputVector);       
	return 1;
}

int vtkFourFlowCollector::RequestInformation(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector) {
	std::cout << "vtkFourFlowCollector::RequestInformation" << std::endl;
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
		outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
		outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
		this->NumberOfTimeSteps = inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
	else
		this->NumberOfTimeSteps = 0;

	return 1;
}

int vtkFourFlowCollector::RequestUpdateExtent(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector) {
	std::cout << "vtkFourFlowCollector::RequestUpdateExtent" << std::endl;
	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	if(inTimes) {
		inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inTimes[this->CurrentTimeIndex]);
		for(int i = 0; i < inputVector[1]->GetNumberOfInformationObjects(); i++) {
			inputVector[1]->GetInformationObject(i)->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inTimes[this->CurrentTimeIndex]);
		}
	}
	return 1;
}

void vtkFourFlowCollector::AddSourceConnection(vtkAlgorithmOutput* input) {
	std::cout << "vtkFourFlowCollector::AddSourceConnection" << std::endl;
	this->AddInputConnection(1, input);
}

void vtkFourFlowCollector::RemoveAllSources() {
	std::cout << "vtkFourFlowCollector::RemoveAllSources" << std::endl;
	this->SetInputConnection(1, 0);
}