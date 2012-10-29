#ifndef __vtkFourFlowCollector_h
#define __vtkFourFlowCollector_h

#include "vtkPVExtractVOI.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkStreamTracer.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkTableAlgorithm.h"
#include "structs.h"
#include "vtkPolyDataAlgorithm.h"

//class vtkFourFlowCollector : public vtkTableAlgorithm {
class vtkFourFlowCollector : public vtkPolyDataAlgorithm {
public:
	static vtkFourFlowCollector* New();
	//vtkTypeMacro(vtkFourFlowCollector,vtkTableAlgorithm);
	vtkTypeMacro(vtkFourFlowCollector,vtkPolyDataAlgorithm);
	
	void AddSourceConnection(vtkAlgorithmOutput* input);
	void RemoveAllSources();
	vtkGetMacro(NumberOfTimeSteps,int);
	bool continueExecuting;
protected:
	int NumberOfTimeSteps;
	int CurrentTimeIndex;
	virtual int FillInputPortInformation(int port, vtkInformation* info);
    virtual int RequestData(vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector);
    virtual int ProcessRequest(vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector);
    virtual int RequestInformation(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector);
    virtual int RequestUpdateExtent(vtkInformation* request,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector);
  vtkFourFlowCollector();
  ~vtkFourFlowCollector();

private:
	int getIntersectedPointsThisFrame(vtkPolyData *particlePolyData, vtkPolyData *ringPolygonData, int currentFrame);
	LineCollection getCollidedLines(LineCollection allLines, vtkPolyData *ringPolygonData);
	LineCollection getCurrentLines(int currentFrame, vtkPolyData *particlePolyData);
	vector<Point> getPoints(vtkPolyData *polygonData);
	PointBuffer pointBuffer;
};


#endif


