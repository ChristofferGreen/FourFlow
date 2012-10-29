#include "vtkFourFlowGraph.h"
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

vtkStandardNewMacro(vtkFourFlowGraph);

vtkFourFlowGraph::vtkFourFlowGraph() : vtkTableAlgorithm() {
	this->SetNumberOfInputPorts(2);
	this->CurrentTimeIndex = 0;
	this->ShowFlow = true;
	this->ShowMaximumVelocity = false;
	this->ShowMeanVelocity = false;
	this->totalFlow = 0.0;
	this->continueExecuting = false;
}

vtkFourFlowGraph::~vtkFourFlowGraph() {
}

int vtkFourFlowGraph::FillInputPortInformation(int port, vtkInformation* info) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");    
	return 1;
}

int vtkFourFlowGraph::RequestData(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector) {
	cout << "test: " << ShowFlow << " " << ShowFlowPrev << " " << ShowMaximumVelocity << " " << ShowMaximumVelocityPrev << " " << ShowMeanVelocity << " " << ShowMeanVelocityPrev << endl;
	if(this->continueExecuting == false)
		if(ShowFlow == ShowFlowPrev && ShowMaximumVelocity == ShowMaximumVelocityPrev && ShowMeanVelocity == ShowMeanVelocityPrev)
			return 1;
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
	vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);
	vtkTable *table = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
	double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	int size = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

	if(this->CurrentTimeIndex == 0) {
		this->totalFlow = 0.0;
	}
	if(this->CurrentTimeIndex < inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS())) {
		/*
			First we insert the columns to the table data structure.
		*/
		int timeColumn = 0;
		if(ShowMeanVelocity) {
			timeColumn++;
			if(!table->GetColumnByName("Mean Velocity [m/s]")) {
				vtkFloatArray *arr = vtkFloatArray::New();
				arr->SetName("Mean Velocity [m/s]");
				table->AddColumn(arr);
			}
		}
		if(ShowMaximumVelocity) {
			timeColumn++;
			if(!table->GetColumnByName("Maximum Velocity [m/s]")) {
				vtkFloatArray *maxVel = vtkFloatArray::New();
				maxVel->SetName("Maximum Velocity [m/s]");
				table->AddColumn(maxVel);
			}
		}
		if(ShowFlow) {
			timeColumn++;
			if(!table->GetColumnByName("Flow [mL/s]")) {
				vtkFloatArray *flow = vtkFloatArray::New();
				flow->SetName("Flow [mL/s]");
				table->AddColumn(flow);
			}
		}

		if(!table->GetColumnByName("Time")) {
			vtkFloatArray *timeCol = vtkFloatArray::New();
			timeCol->SetName("Time");
			table->AddColumn(timeCol);
		}

		table->SetNumberOfRows(size);


		/*
			Here we remove the points that form the polygon since they are not part of the grid data
		*/
		float maxVelocity = 0.0;
		float velocity = 0.0;
		float flow = 0.0;
		int points = 0;
		set<int> pointSet;
		vtkPolyData *polyData = vtkPolyData::SafeDownCast(inInfo2->Get(vtkDataObject::DATA_OBJECT()));
		if(polyData && polyData->GetPoints()) {
			for(int i = 0; i < polyData->GetPoints()->GetNumberOfPoints(); i++)
				pointSet.insert(i);
			for(int i = 0; i < polyData->GetNumberOfCells(); i++) {
				vtkCell *cell = polyData->GetCell(i);
				for(int j = 0; j < cell->GetNumberOfPoints(); j++) {
					pointSet.erase(cell->PointIds->GetId(j));
				}
			}
			points = pointSet.size();
		}
		if(polyData && polyData->GetPoints() && points > 3) {
			cout << "polyData->GetPoints()->GetNumberOfPoints(): " << polyData->GetPoints()->GetNumberOfPoints() << endl;
			/*
				Here we calculate the normal of the grid
			*/
			double normal[3];
			vtkPolygon *polygon = vtkPolygon::SafeDownCast(polyData->GetCell(0));
			if(!polygon || !polygon->GetPoints()->GetNumberOfPoints())
				return 0;
			polygon->ComputeNormal(polygon->GetPoints(), normal);
			
			/*
				We also must know the area of the cell surrounding a point, we do this by taking 
				the length between the two closest points on the grid.
			*/
			double areaPos1[3], areaPos2[3];
			set<int>::iterator iter = pointSet.begin();
			polyData->GetPoints()->GetPoint(*iter, areaPos1);iter++;
			double cellSize = -1;
			for(; iter != pointSet.end(); iter++) {
				double areaVector[3];
				polyData->GetPoints()->GetPoint(*iter, areaPos2);
				areaVector[0] = areaPos1[0]-areaPos2[0];
				areaVector[1] = areaPos1[1]-areaPos2[1];
				areaVector[2] = areaPos1[2]-areaPos2[2];

				double tmpCellSize = sqrt(areaVector[0]*areaVector[0]+areaVector[1]*areaVector[1]+areaVector[2]*areaVector[2]);
				if(cellSize == -1 || tmpCellSize < cellSize)
					cellSize = tmpCellSize;
			}
			double area = cellSize*cellSize;

			/*
				Now we start calculating the flow
			*/
			double dt;
			if(this->CurrentTimeIndex < size-1)
				dt = inTimes[this->CurrentTimeIndex+1]-inTimes[this->CurrentTimeIndex];
			else
				dt = inTimes[this->CurrentTimeIndex-1]-inTimes[this->CurrentTimeIndex-2];

			vtkStructuredGrid *structuredGrid = vtkStructuredGrid::SafeDownCast(vtkMultiBlockDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()))->GetBlock(0));
			if(!structuredGrid)
				return 0;
			for(iter = pointSet.begin(); iter != pointSet.end(); iter++) {
				int pointId = *iter;
				double pointCoordinates[3];
				polyData->GetPoints()->GetPoint(pointId, pointCoordinates);

				float velocitySize = 0.0;
				double pcoords[3];
				double weights[8];
				vtkIdType cellId = -1;
				vtkIdType subId = -1;
				double distanceSquared;
				cellId = structuredGrid->FindCell(pointCoordinates,NULL,cellId,0.0001,subId,pcoords,weights);
				if(cellId > -1) {
					vtkCell *cell = structuredGrid->GetCell(cellId);
					double pointVelocity[3] = {0.0, 0.0, 0.0};
					for(int i = 0; i < 8; i++) {
						vtkIdType pointId = cell->PointIds->GetId(i);
						pointVelocity[0] = 
							pointVelocity[0]+structuredGrid->GetPointData()->GetVectors()->GetTuple3(pointId)[0]*weights[i];
						pointVelocity[1] = 
							pointVelocity[1]+structuredGrid->GetPointData()->GetVectors()->GetTuple3(pointId)[1]*weights[i];
						pointVelocity[2] = 
							pointVelocity[2]+structuredGrid->GetPointData()->GetVectors()->GetTuple3(pointId)[2]*weights[i];
					}
					float tmpVelocity = vtkMath::Normalize(pointVelocity);
					velocity += tmpVelocity;
					if(tmpVelocity > maxVelocity)
						maxVelocity = tmpVelocity;
					double dot = vtkMath::Dot(pointVelocity,normal);

					flow += area*dot*tmpVelocity;
				}
				else {
					cout << "no cell found" << endl;
				}
			}
			velocity = velocity/(float)points;
		}
		if(ShowMeanVelocity)
			table->SetValue(this->CurrentTimeIndex, 0, velocity);
		if(ShowMaximumVelocity) {
			int i = 0;
			if(ShowMeanVelocity)
				i++;
			table->SetValue(this->CurrentTimeIndex, i, maxVelocity);
		}
		if(ShowFlow) {
			int i = 0;
			if(ShowMeanVelocity)
				i++;
			if(ShowMaximumVelocity)
				i++;
			flow = flow*1000000;
			table->SetValue(this->CurrentTimeIndex, i, flow);
			//cout << "time: " << this->CurrentTimeIndex << " points: " << points << " area: " << area << " totalFlow: " << totalFlow << " flow: " << flow << endl;
			cout << "flow: " << flow << endl;
			if(flow > 0)
				totalFlow = totalFlow+flow;
			else
				totalFlow = totalFlow-flow;
		}
		for(int i = 0; i < size; i++)
			table->SetValue(i, timeColumn, inTimes[i]);


		request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
		this->continueExecuting = true;
		this->UpdateProgress(static_cast<double>(this->CurrentTimeIndex)/size);
		this->CurrentTimeIndex++;
	}
	else {
		// flip the graph to be pointing upwards
		double positive = 0;
		double negative = 0;
		if(ShowFlow) {
			int timeColumn = 0;
			if(ShowMeanVelocity)
				timeColumn++;
			if(ShowMaximumVelocity)
				timeColumn++;
			for(int i = 0; i < size; i++) {
				double value = table->GetValue(i, timeColumn).ToDouble();
				if(value > positive)
					positive = value;
				if(value < negative)
					negative = value;
			}
			if((-negative) > positive) {
				for(int i = 0; i < size; i++)
					table->SetValue(i, timeColumn, -table->GetValue(i, timeColumn).ToDouble());
			}
			if(this->totalFlow < 0)
				this->totalFlow = -this->totalFlow;
			// add total flow
			if(table->GetColumnByName("Flow [mL/s]")) {
				double dt;
				if(this->CurrentTimeIndex < size-1)
					dt = inTimes[this->CurrentTimeIndex+1]-inTimes[this->CurrentTimeIndex];
				else
					dt = inTimes[this->CurrentTimeIndex-1]-inTimes[this->CurrentTimeIndex-2];
				totalFlow = totalFlow*dt;
				std::ostringstream buff;
				buff<<totalFlow;
				vtkFloatArray *flow = vtkFloatArray::New();
				table->GetColumnByName("Flow [mL/s]")->SetName((string("Flow [mL/s], Total Flow: ") + buff.str() + string(" [mL]")).c_str());
			}
		}
		request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
		this->continueExecuting = false;
		table->Update();
		this->CurrentTimeIndex = 0;
	}
	ShowFlowPrev = ShowFlow;
	ShowMaximumVelocityPrev = ShowMaximumVelocity;
	ShowMeanVelocityPrev = ShowMeanVelocity;
	return 1;
}

int vtkFourFlowGraph::ProcessRequest(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector) {
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

int vtkFourFlowGraph::RequestInformation(vtkInformation* request,
                              vtkInformationVector** inputVector,
                              vtkInformationVector* outputVector) {
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

int vtkFourFlowGraph::RequestUpdateExtent(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector) {
	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	vtkInformation *inInfo2 = inputVector[1]->GetInformationObject(0);
	double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	if(inTimes) {
		inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), &inTimes[this->CurrentTimeIndex], 1);
		inInfo2->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), &inTimes[this->CurrentTimeIndex], 1);
	}
	return 1;
}

void vtkFourFlowGraph::AddSourceConnection(vtkAlgorithmOutput* input) {
	this->AddInputConnection(1, input);
}

void vtkFourFlowGraph::RemoveAllSources() {
	this->SetInputConnection(1, 0);
}