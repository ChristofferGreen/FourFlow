#ifndef ImportDirectoryWindow_h
#define ImportDirectoryWindow_h

#include "ui_ImportDirectoryWindow.h"
#include "qtIncludes.h"
#include "pqIncludes.h"
#include "cppIncludes.h"

class FourFlowMainWindow;

class ImportDirectoryWindow : public QMainWindow {
	Q_OBJECT
	typedef QMainWindow Superclass;
	public:
		FourFlowMainWindow *ffWindow;
		QSharedPointer<Ui_ImportDirectoryWindow> internals;

		ImportDirectoryWindow();
		~ImportDirectoryWindow() {}
	protected slots:
		void importSelectedFilesSlot();
		void createVortexSlot();
		void updatedLoadDataSlot(pqPipelineSource *source);
		void representationAddedLoadDataSlot(pqPipelineSource *source, pqDataRepresentation *repre, int val);
		void updatedLoadDataPostRepresentationSlot();
		void representationAddedStreamTracer(pqPipelineSource *source, pqDataRepresentation *repr, int val);
		void representationAddedVortex(pqPipelineSource *source, pqDataRepresentation *repre, int val);
	private:
		bool createVortex, SAX, endo;
		pqPipelineSource *vortexSource;
		pqPipelineSource *vortexInput;
		pqPipelineSource *vortexIsoVolume;
		list<string> caseFiles;
		list<string> pvdFiles;
		string pvdPath;

		pqDataRepresentation *dataRepresentation;
		void decodeCaseFiles();
		void loadTexture(const QString& filename, pqDataRepresentation *Representation);
		void checkAnimationStepSanity();
};

#endif