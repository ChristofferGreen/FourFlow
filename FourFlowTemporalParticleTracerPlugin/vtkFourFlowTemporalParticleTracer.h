#ifndef __vtkFourFlowTemporalParticleTracer_h
#define __vtkFourFlowTemporalParticleTracer_h

#include "vtkTemporalStreamTracer.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkStreamTracer.h"

class vtkFourFlowTemporalParticleTracer : public vtkTemporalStreamTracer {
public:
	static vtkFourFlowTemporalParticleTracer* New();
	vtkTypeMacro(vtkFourFlowTemporalParticleTracer,vtkTemporalStreamTracer);

	vtkSetMacro(ParticleReleaseStartFrame, int);
    vtkGetMacro(ParticleReleaseStartFrame, int);

    vtkSetMacro(ParticleReleaseEndFrame, int);
    vtkGetMacro(ParticleReleaseEndFrame, int);

protected:
	int ParticleReleaseStartFrame;
	int ParticleReleaseEndFrame;
//BTX
	void IntegrateParticleCustom(vtkTemporalStreamTracerNamespace::ParticleListIterator &it, 
								double currenttime, double targettime,
								vtkInitialValueProblemSolver* integrator);
//ETX

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

  vtkFourFlowTemporalParticleTracer();
  ~vtkFourFlowTemporalParticleTracer();
};


#endif


