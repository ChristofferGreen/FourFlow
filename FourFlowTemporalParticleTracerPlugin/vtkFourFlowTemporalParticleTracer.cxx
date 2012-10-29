#include "vtkFourFlowTemporalParticleTracer.h"
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

using namespace vtkTemporalStreamTracerNamespace;

vtkStandardNewMacro(vtkFourFlowTemporalParticleTracer);

vtkFourFlowTemporalParticleTracer::vtkFourFlowTemporalParticleTracer() : vtkTemporalStreamTracer() {
	this->SetIntegratorType(RUNGE_KUTTA2);
}

vtkFourFlowTemporalParticleTracer::~vtkFourFlowTemporalParticleTracer() {
}

int vtkFourFlowTemporalParticleTracer::RequestData(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector) {
	//this->SetIntegrationDirection(BACKWARD);
	//cout << "vtkFourFlowTemporalParticleTracer::RequestData TimeStep: " << this->TimeStep << " ActualTimeStep: " << this->ActualTimeStep << endl;
	vtkInformation *outInfo = outputVector->GetInformationObject(0);
	//return vtkTemporalStreamTracer::RequestData(request, inputVector, outputVector);
	//
	// Parallel/Piece information
	//
	this->UpdatePiece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
	this->UpdateNumPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

	//
	// Inputs information
	//
	int numInputs = inputVector[0]->GetNumberOfInformationObjects();

	// inherited from streamtracer, make sure it is null
	this->InputData = NULL;

	// how many temporal inputs were we given, 
	// from each - add to our list of blocks
	this->InputDataT[0] = NULL;
	this->InputDataT[1] = NULL;
	for (int idx=0; idx<numInputs; ++idx)
	{
		vtkDataObject      *input = 0;
		vtkTemporalDataSet    *td = 0;
		vtkInformation    *inInfo = inputVector[0]->GetInformationObject(idx);
		if (inInfo)
		{
			input = inInfo->Get(vtkDataObject::DATA_OBJECT());
			td    = vtkTemporalDataSet::SafeDownCast(input);
			if (td && this->AddTemporalInput(td)) {
				{
					//
					// Get the timestep information for this instant
					//
					vtkInformation *doInfo = td->GetInformation();
					vtkstd::vector<double> timesteps;
					if (doInfo->Has(vtkDataObject::DATA_TIME_STEPS()))
					{
						int NumberOfDataTimeSteps = doInfo->Length(vtkDataObject::DATA_TIME_STEPS());
						if (NumberOfDataTimeSteps<2) {
							vtkErrorMacro(<<"Not enough time steps in input data");
							return 1;
						}
						timesteps.resize(NumberOfDataTimeSteps);
						doInfo->Get(vtkDataObject::DATA_TIME_STEPS(), &timesteps[0]);
					}
					else {
						vtkErrorMacro(<<"No time step info");
						return 1;
					}
					this->CurrentTimeSteps[0] = timesteps[0]*this->TimeStepResolution;
					this->CurrentTimeSteps[1] = timesteps[1]*this->TimeStepResolution;
				}
			}
			else 
			{
				vtkDebugMacro(<<"AddTemporalDataSet failed : Input dataset was not temporal or had insufficient timesteps");
				return 1;
			}
		}
	}

	//
	// How many Seed point sources are connected?
	// Copy the sources into a vector for later use
	//

	int numSources = inputVector[1]->GetNumberOfInformationObjects();
	vtkstd::vector<vtkDataSet*> SeedSources;
	for (int idx=0; idx<numSources; ++idx)
	{
		vtkDataObject     *dobj   = 0;
		vtkInformation    *inInfo = inputVector[1]->GetInformationObject(idx);
		if (inInfo)
		{
			dobj   = inInfo->Get(vtkDataObject::DATA_OBJECT());
			SeedSources.push_back(vtkDataSet::SafeDownCast(dobj));
		}
	}

	/*if (this->IntegrationDirection != FORWARD) 
	{
	vtkErrorMacro(<<"We can only handle forward time particle tracking at the moment");
	return 1;
	}*/

	//
	// Add the datasets to an interpolator object
	//
	if (this->InitializeInterpolator() != VTK_OK)
	{
		if (this->InputDataT[0]) this->InputDataT[0] = NULL;
		if (this->InputDataT[1]) this->InputDataT[1] = NULL;
		vtkErrorMacro(<<"InitializeInterpolator failed");
		return 1;
	}

	//
	// Setup some variables
	//
	vtkSmartPointer<vtkInitialValueProblemSolver> integrator;
	integrator.TakeReference(this->GetIntegrator()->NewInstance());
	integrator->SetFunctionSet(this->Interpolator);

	//
	// Make sure the Particle Positions are initialized with Seed particles
	//
	this->ReinjectionFlag = 0;

	if((this->ActualTimeStep >= this->ParticleReleaseStartFrame || this->ParticleReleaseStartFrame == -1) && 
		(this->ActualTimeStep <= this->ParticleReleaseEndFrame || this->ParticleReleaseEndFrame == -1)) {
		if (this->ForceReinjectionEveryNSteps>0) {
			if ((this->ActualTimeStep%this->ForceReinjectionEveryNSteps)==0) 
			{
				this->ReinjectionFlag = 1;
			}
		}
	}
	//
	// If T=0 reset everything to allow us to setup stuff then start an animation
	// with a clean slate
	//
	if (this->ActualTimeStep==0) {
		this->LocalSeeds.clear();
		this->ParticleHistories.clear();
		this->EarliestTime       =-1E6;
		if((this->ActualTimeStep >= this->ParticleReleaseStartFrame || this->ParticleReleaseStartFrame == -1) && 
			(this->ActualTimeStep <= this->ParticleReleaseEndFrame || this->ParticleReleaseEndFrame == -1)) {
			this->ReinjectionFlag    = 1;
		}
		this->ReinjectionCounter = 0;
		this->UniqueIdCounter    = 0;
		this->UniqueIdCounterMPI = 0;
	}
	else if (this->CurrentTimeSteps[0]<this->EarliestTime) {
		//
		// We don't want to go back in time, so just reuse whatever we have
		//
		vtkDebugMacro("skipping particle tracking because we have seen this timestep before");
		outInfo->Set(vtkDataObject::DATA_TIME_STEPS(), 
			&this->OutputTimeValues[this->ActualTimeStep], 1);
		if (this->InputDataT[0]) this->InputDataT[0] = NULL;
		if (this->InputDataT[1]) this->InputDataT[1] = NULL;
		return 1;
	}
	this->EarliestTime = (this->CurrentTimeSteps[0]>this->EarliestTime)
		? this->CurrentTimeSteps[0] : this->EarliestTime;
	//
	//
	//
	for (unsigned int i=0; i<SeedSources.size(); i++) 
	{
		if (SeedSources[i]->GetMTime()>this->ParticleInjectionTime)
		{
			//    this->ReinjectionFlag = 1;
		}
	}

	//
	// Lists for seed particles
	//
	ParticleVector candidates;
	ParticleVector received;
	//

	if (this->ReinjectionFlag) {
		int seedPointId=0;
		if (this->StaticSeeds && this->AllFixedGeometry && this->LocalSeeds.size()==0) {
			for (unsigned int i=0; i<SeedSources.size(); i++) {
				this->AssignSeedsToProcessors(SeedSources[i], i, 0, this->LocalSeeds, seedPointId);
	  }
		}
		else {
			// wipe the list and reclassify for each injection
			this->LocalSeeds.clear();
			for (unsigned int i=0; i<SeedSources.size(); i++) {
				this->AssignSeedsToProcessors(SeedSources[i], i, 0, this->LocalSeeds, seedPointId);
	  }
		}
		this->ParticleInjectionTime.Modified();

		// Now update our main list with the ones we are keeping
		vtkDebugMacro(<< "Reinjection about to update candidates (" << this->LocalSeeds.size() << " particles)");
		this->UpdateParticleList(this->LocalSeeds);
		this->ReinjectionCounter += 1;
	}

	//
	// setup all our output arrays
	//
	vtkDebugMacro(<< "About to allocate point arrays ");
	vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
	this->ParticleAge         = vtkSmartPointer<vtkFloatArray>::New();
	this->ParticleIds         = vtkSmartPointer<vtkIntArray>::New();
	this->ParticleSourceIds   = vtkSmartPointer<vtkCharArray>::New();
	this->InjectedPointIds    = vtkSmartPointer<vtkIntArray>::New();
	this->InjectedStepIds     = vtkSmartPointer<vtkIntArray>::New();
	this->ErrorCode           = vtkSmartPointer<vtkIntArray>::New();
	this->ParticleVorticity   = vtkSmartPointer<vtkFloatArray>::New();
	this->ParticleRotation    = vtkSmartPointer<vtkFloatArray>::New();
	this->ParticleAngularVel  = vtkSmartPointer<vtkFloatArray>::New();
	this->cellVectors         = vtkSmartPointer<vtkDoubleArray>::New();
	this->ParticleCells       = vtkSmartPointer<vtkCellArray>::New();
	this->OutputCoordinates   = vtkSmartPointer<vtkPoints>::New();
	this->OutputPointData     = output->GetPointData();
	this->OutputPointData->Initialize();
	this->InterpolationCount  = 0;
	vtkDebugMacro(<< "About to Interpolate allocate space");
	this->OutputPointData->InterpolateAllocate(this->DataReferenceT[1]->GetPointData());
	//
	this->ParticleAge->SetName("ParticleAge");
	this->ParticleIds->SetName("ParticleId");
	this->ParticleSourceIds->SetName("ParticleSourceId");
	this->InjectedPointIds->SetName("InjectedPointId");
	this->InjectedStepIds->SetName("InjectionStepId");
	this->ErrorCode->SetName("ErrorCode");

	if (this->ComputeVorticity)	{
		this->cellVectors->SetNumberOfComponents(3);
		this->cellVectors->Allocate(3*VTK_CELL_SIZE);   
		this->ParticleVorticity->SetName("Vorticity");
		this->ParticleRotation->SetName("Rotation");
		this->ParticleAngularVel->SetName("AngularVelocity");
	}

	output->SetPoints(this->OutputCoordinates);
	output->SetVerts(this->ParticleCells);
	vtkDebugMacro(<< "Finished allocating point arrays ");

	//
	// Perform 2 passes
	// Pass 0 : Integration of particles created by a source in this process 
	// or received at start from a source in another process.
	//
	// Pass 1 : Particles that were sent in mid integration from another process
	// are added in and their integration continued here. In actual fact, the process
	// should be repeated until all particles are finished, but the chances of 
	// a particle stepping inside and out again through a single domain 
	// in one time step are small (hopefully!)

	vtkDebugMacro(<< "Clear MPI send list ");
	this->MPISendList.clear();

	int Number = static_cast<int>(this->ParticleHistories.size());
	ParticleListIterator  it_first = this->ParticleHistories.begin();
	ParticleListIterator  it_last  = this->ParticleHistories.end();
	ParticleListIterator  it_next;
#define PASSES 2
	for (int pass=0; pass<PASSES; pass++) {
		vtkDebugMacro(<<"Begin Pass " << pass << " with " << Number << " Particles");
		for (ParticleListIterator it=it_first; it!=it_last;)
		{
			// Keep the 'next' iterator handy because if a particle is terminated 
			// or leaves the domain, the 'current' iterator will be deleted.
			it_next = it;
			it_next++;
			//
			// Shall we terminate this particle
			//
			double interval = (this->CurrentTimeSteps[1]-this->CurrentTimeSteps[0]);
			bool terminated = false;
			if (this->TerminationTime>0) 
	  {
		  if (this->TerminationTimeUnit == TERMINATION_TIME_UNIT &&
			  (it->age+interval)>this->TerminationTime) {
				  terminated = true;
		  }
		  else if (this->TerminationTimeUnit == TERMINATION_STEP_UNIT &&
			  (it->TimeStepAge+1)>this->TerminationTime) {
				  terminated = true;
		  }
	  }
			if (terminated) {
				this->ParticleHistories.erase(it);
	  }
			else {
				this->IntegrateParticleCustom(it, this->CurrentTimeSteps[0], this->CurrentTimeSteps[1], integrator);
	  }
			//
			if (this->GetAbortExecute()) {
				break;
	  }
			it = it_next;
		}
		// Particles might have been deleted during the first pass as they move 
		// out of domain or age. Before adding any new particles that are sent 
		// to us, we must know the starting point ready for the second pass
		bool list_valid = (this->ParticleHistories.size()>0);
		if (list_valid) {
			// point to one before the end
			it_first = --this->ParticleHistories.end();
		}
		// Send and receive any particles which exited/entered the domain
		if (this->UpdateNumPieces>1 && pass<(PASSES-1)) {
			// the Particle lists will grow if any are received
			// so we must be very careful with our iterators
			vtkDebugMacro(<<"End of Pass " << pass << " with " 
				<< this->ParticleHistories.size() << " " 
				<< " about to Transmit/Receive " << this->MPISendList.size());
			this->TransmitReceiveParticles(this->MPISendList, received, true);
			// don't want the ones that we sent away
			this->MPISendList.clear();
			int assigned;
			// classify all the ones we received
			if (received.size()>0) {
				this->TestParticles(received, candidates, assigned);
				vtkDebugMacro(<<"received " << received.size() << " : assigned locally " << assigned);
				received.clear();
	  }
			// Now update our main list with the ones we are keeping
			this->UpdateParticleList(candidates);
			// free up unwanted memory
			Number = static_cast<int>(candidates.size());
			candidates.clear();
		}
		it_last = this->ParticleHistories.end();
		if (list_valid) {
			// increment to point to first new entry
			it_first++;
		}
		else {
			it_first = this->ParticleHistories.begin();
		}
	}
	if (this->MPISendList.size()>0) {
		// If a particle went out of domain on the second pass, it should be sent
		// can it really pass right through a domain in one step?
		// what about grazing the edge of rotating zone?
		vtkDebugMacro(<< "MPISendList not empty " << this->MPISendList.size());
		this->MPISendList.clear();
	}

	//
	// We must only add these scalar arrays at the end because the 
	// existing scalars on the input get interpolated during iteration
	// over the particles
	// 
	this->OutputPointData->AddArray(this->ParticleIds);
	this->OutputPointData->AddArray(this->ParticleSourceIds);
	this->OutputPointData->AddArray(this->InjectedPointIds);
	this->OutputPointData->AddArray(this->InjectedStepIds);
	this->OutputPointData->AddArray(this->ErrorCode);
	this->OutputPointData->AddArray(this->ParticleAge);
	if (this->ComputeVorticity)
	{
		this->OutputPointData->AddArray(this->ParticleVorticity);
		this->OutputPointData->AddArray(this->ParticleRotation);
		this->OutputPointData->AddArray(this->ParticleAngularVel);
	}

	if (this->InterpolationCount!=this->OutputCoordinates->GetNumberOfPoints()) {
		vtkErrorMacro(<< "Mismatch in point array/data counts");
	}
	//
	outInfo->Set(vtkDataObject::DATA_TIME_STEPS(), 
		&this->OutputTimeValues[this->ActualTimeStep], 1);

	// save some locator building, by re-using them as time progresses
	this->Interpolator->AdvanceOneTimeStep();

	//
	// Let go of inputs 
	//
	if (this->InputDataT[0]) this->InputDataT[0] = NULL;;
	if (this->InputDataT[1]) this->InputDataT[1] = NULL;;

	//
	// Write Particles out if necessary
	//
	// NB. We don't want our writer to trigger any updates, 
	// so shallow copy the output
	if (this->ParticleWriter && this->EnableParticleWriting) {
		vtkSmartPointer<vtkPolyData> polys = vtkSmartPointer<vtkPolyData>::New();
		polys->ShallowCopy(output);
		int N = polys->GetNumberOfPoints();
		this->ParticleWriter->SetFileName(this->ParticleFileName);
		this->ParticleWriter->SetTimeStep(this->ActualTimeStep);
		this->ParticleWriter->SetTimeValue(this->CurrentTimeSteps[1]);
		this->ParticleWriter->SetInput(polys);
		this->ParticleWriter->Write();
		this->ParticleWriter->CloseFile();
		this->ParticleWriter->SetInput(NULL);
#ifdef VTK_USE_MPI
		this->Controller->Barrier();
#endif
		vtkDebugMacro(<< "Written " << N);
	}
	//  this->Interpolator->ShowCacheResults();
	//  vtkErrorMacro(<<"RequestData done");  
	return 1;
}

void vtkFourFlowTemporalParticleTracer::IntegrateParticleCustom(ParticleListIterator &it, double currenttime, double targettime, vtkInitialValueProblemSolver* integrator) {
	double epsilon = (targettime-currenttime)/100.0;
	double velocity[3], point1[4], point2[4] = {0.0, 0.0, 0.0, 0.0};
	double minStep=0, maxStep=0;
	double stepWanted, stepTaken=0.0;
	substeps = 0;

	ParticleInformation &info = (*it);
	// Get the Initial point {x,y,z,t}
	memcpy(point1, &info.CurrentPosition, sizeof(Position));

	if (point1[3]<(currenttime-epsilon) || point1[3]>(targettime+epsilon)) {
		vtkDebugMacro(<< "Bad particle time : expected (" 
			<< this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] 
		<< ") got " << point1[3]);
	}

	IntervalInformation delT;
	delT.Unit     = LENGTH_UNIT;
	delT.Interval = (targettime-currenttime) * this->InitialIntegrationStep;
	epsilon = delT.Interval*1E-3;

	//
	// begin interpolation between available time values, if the particle has 
	// a cached cell ID and dataset - try to use it, 
	//
	this->Interpolator->SetCachedCellIds(info.CachedCellId, info.CachedDataSetId);

	bool particle_good = true;
	info.ErrorCode = 0;
	while (point1[3] < (targettime-epsilon)) {
		//
		// Here beginneth the real work
		//
		double error = 0;

		// If, with the next step, propagation will be larger than
		// max, reduce it so that it is (approximately) equal to max.
		if(this->IntegrationDirection == FORWARD)
			stepWanted = delT.Interval;
		else
			stepWanted = -delT.Interval;
		if(this->IntegrationDirection == FORWARD)
		{
			if ( (point1[3] + stepWanted) > targettime )
			{
				stepWanted = targettime - point1[3];
				maxStep = stepWanted;
			}
		}
		else
		{
			if ( (point1[3] + -stepWanted) > targettime )
			{
				stepWanted = -(targettime - point1[3]);
				maxStep = stepWanted;
			}
		}
		this->LastUsedStepSize = stepWanted;

		// Calculate the next step using the integrator provided.
		// If the next point is out of bounds, send it to another process
		int res = 0;
		if(this->IntegrationDirection == FORWARD)
		{
			res = integrator->ComputeNextStep(
				point1, point2, point1[3], stepWanted, 
				stepTaken, minStep, maxStep, 
				this->MaximumError, error);
		}
		else
		{
			res = integrator->ComputeNextStep(
				point1, point2, point1[3], stepWanted, 
				stepTaken, -minStep, -maxStep, 
				this->MaximumError, error);
		}
		if (res != 0)
		{
			// if the particle is sent, remove it from the list
			info.ErrorCode = 1;
			if (this->SendParticleToAnotherProcess(info, point1, this->LastUsedStepSize)) {
				this->ParticleHistories.erase(it);
				particle_good = false;
				break;
			} 
			else 
			{
				// particle was not sent, retry saved it, so copy info back
				substeps++;
				memcpy(point1, &info.CurrentPosition, sizeof(Position));  
			}
		}
		else // success, increment position/time
		{
			//	cout << "stepWanted: " << stepWanted << endl;
			substeps++;

			// increment the particle time
			if(this->IntegrationDirection == FORWARD)
			{
				point2[3] = point1[3] + stepTaken;
				info.age += stepTaken;
			}
			else
			{
				point2[3] = point1[3] + -stepTaken;
				info.age += -stepTaken;
			}

			// Point is valid. Insert it.
			memcpy(&info.CurrentPosition, point2, sizeof(Position));
			memcpy(point1, point2, sizeof(Position));
		}

		// If the solver is adaptive and the next time step (delT.Interval)
		// that the solver wants to use is smaller than minStep or larger 
		// than maxStep, re-adjust it. This has to be done every step
		// because minStep and maxStep can change depending on the Cell
		// size (unless it is specified in time units)
		if (integrator->IsAdaptive())
		{
			// code removed. Put it back when this is stable
		}
	}
	if (particle_good) {
		// The integration succeeded, but check the computed final position 
		// is actually inside the domain (the intermediate steps taken inside 
		// the integrator were ok, but the final step may just pass out)
		// if it moves out, we can't interpolate scalars, so we must send it away
		info.LocationState = this->Interpolator->TestPoint(info.CurrentPosition.x);
		if (info.LocationState==ID_OUTSIDE_ALL)
		{
			info.ErrorCode = 2;
			// if the particle is sent, remove it from the list
			if (this->SendParticleToAnotherProcess(info, point1, this->LastUsedStepSize)) {
				this->ParticleHistories.erase(it);
				particle_good = false;
			}
		}
	}

	//
	// Has this particle stagnated
	//
	if (particle_good) {
		this->Interpolator->GetLastGoodVelocity(velocity);
		info.speed = vtkMath::Norm(velocity);
		if (it->speed <= this->TerminalSpeed)
		{
			this->ParticleHistories.erase(it);
			particle_good = false;
		}
	}

	//
	// We got this far without error :
	// Insert the point into the output
	// Create any new scalars and interpolate existing ones
	// Cache cell ids and datasets
	//
	if (particle_good) {
		//
		// store the last Cell Ids and dataset indices for next time particle is updated
		//
		this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
		//
		info.TimeStepAge += 1;
		//
		// Now generate the output geometry and scalars
		//
		double    *coord = &info.CurrentPosition.x[0];
		vtkIdType tempId = this->OutputCoordinates->InsertNextPoint(coord);
		// create the cell
		this->ParticleCells->InsertNextCell(1, &tempId);
		// set the easy scalars for this particle
		this->ParticleIds->InsertNextValue(info.UniqueParticleId);
		this->ParticleSourceIds->InsertNextValue(info.SourceID);
		this->InjectedPointIds->InsertNextValue(info.InjectedPointId);
		this->InjectedStepIds->InsertNextValue(info.InjectedStepId);
		this->ErrorCode->InsertNextValue(info.ErrorCode);
		this->ParticleAge->InsertNextValue(info.age);
		//
		// Interpolate all existing point attributes
		// In principle we always integrate the particle until it reaches Time2 
		// - so we don't need to do any interpolation of the scalars 
		// between T0 and T1, just fetch the values
		// of the spatially interpolated scalars from T1.
		//
		if (info.LocationState==ID_OUTSIDE_T1) {
			this->Interpolator->InterpolatePoint(0, this->OutputPointData, tempId);
		}
		else {
			this->Interpolator->InterpolatePoint(1, this->OutputPointData, tempId);
		}
		this->InterpolationCount++;
		//
		// Compute vorticity 
		//
		if (this->ComputeVorticity)
		{
			vtkGenericCell *cell;
			double pcoords[3], vorticity[3], weights[256];
			double rotation, omega;
			// have to use T0 if particle is out at T1, otherwise use T1
			if (info.LocationState==ID_OUTSIDE_T1) {
				this->Interpolator->GetVorticityData(
					0, pcoords, weights, cell, this->cellVectors);
			}
			else {
				this->Interpolator->GetVorticityData(
					1, pcoords, weights, cell, this->cellVectors);
			}
			vtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vorticity);
			this->ParticleVorticity->InsertNextTuple(vorticity);
			// local rotation = vorticity . unit tangent ( i.e. velocity/speed )
			if (info.speed != 0.0) {
				omega = vtkMath::Dot(vorticity, velocity);
				omega /= info.speed;
				omega *= this->RotationScale;
			}
			else {
				omega = 0.0;
			}     
			vtkIdType index = this->ParticleAngularVel->InsertNextValue(omega);
			if (index>0) {     
				rotation     = info.rotation + (info.angularVel + omega)/2 * (info.CurrentPosition.x[3] - info.time);
			} 
			else {
				rotation     = 0.0;
			}
			this->ParticleRotation->InsertNextValue(rotation);
			info.rotation   = rotation;
			info.angularVel = omega;
			info.time       = info.CurrentPosition.x[3];
		}
	}
	else this->Interpolator->ClearCache();

	double eps = (this->CurrentTimeSteps[1]-this->CurrentTimeSteps[0])/100;
	if (point1[3]<(this->CurrentTimeSteps[0]-eps) || point1[3]>(this->CurrentTimeSteps[1]+eps)) {
		vtkDebugMacro(<< "Unexpected time ending IntegrateParticle - expected (" << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] << ") got " << point1[3]);  
	}
}

int vtkFourFlowTemporalParticleTracer::ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector) {
	return vtkTemporalStreamTracer::ProcessRequest(request, inputVector, outputVector);
}

int vtkFourFlowTemporalParticleTracer::RequestInformation(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector) {
	return vtkTemporalStreamTracer::RequestInformation(request, inputVector, outputVector);
}

int vtkFourFlowTemporalParticleTracer::RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector){
	return vtkTemporalStreamTracer::RequestUpdateExtent(request, inputVector, outputVector);
}
