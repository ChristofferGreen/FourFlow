#ifndef __vtkFourFlowGraph_h
#define __vtkFourFlowGraph_h

#include "vtkPVExtractVOI.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkStreamTracer.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkTableAlgorithm.h"


class vtkFourFlowGraph : public vtkTableAlgorithm
{
public:
	static vtkFourFlowGraph* New();
	vtkTypeMacro(vtkFourFlowGraph,vtkTableAlgorithm);
	
	void AddSourceConnection(vtkAlgorithmOutput* input);
	void RemoveAllSources();
	vtkGetMacro(NumberOfTimeSteps,int);
	bool ShowFlow;
	bool ShowFlowPrev;
	bool ShowMaximumVelocity;
	bool ShowMaximumVelocityPrev;
	bool ShowMeanVelocity;
	bool ShowMeanVelocityPrev;
	bool continueExecuting;
	vtkSetMacro(ShowFlow, int);
	vtkGetMacro(ShowFlow, int);
	vtkBooleanMacro(ShowFlow, int);

	vtkSetMacro(ShowMaximumVelocity, int);
	vtkGetMacro(ShowMaximumVelocity, int);
	vtkBooleanMacro(ShowMaximumVelocity, int);

	vtkSetMacro(ShowMeanVelocity, int);
	vtkGetMacro(ShowMeanVelocity, int);
	vtkBooleanMacro(ShowMeanVelocity, int);
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
  vtkFourFlowGraph();
  ~vtkFourFlowGraph();
  float totalFlow;
};


#endif


