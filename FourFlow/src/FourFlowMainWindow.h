#ifndef __FourFlowMainWindow_h 
#define __FourFlowMainWindow_h

#include "ImportDirectoryWindow.h"
#include "qtIncludes.h"
class pqInternals;

class FourFlowMainWindow : public QMainWindow {
  Q_OBJECT
  typedef QMainWindow Superclass;
  public:
    QSharedPointer<pqInternals> internals;
	string preferedColorVariable;

    FourFlowMainWindow();
    ~FourFlowMainWindow();
	void useVelocityForColor(pqPipelineSource *source);
	void setColorMapGreyscale(pqDataRepresentation *repr);
	void setColorMapRed(pqDataRepresentation *repr);
  protected slots:
	void importDir();
	void representationAddedGreyscale(pqPipelineSource *source, pqDataRepresentation *repr, int val);
	void representationAddedSurface(pqPipelineSource *source, pqDataRepresentation *repr, int val);
	void setColorMapGreyscale();
	void setRepresentationToSurface();
	void nextTimeStepShortCut();
	void previousTimeStepShortCut();
  private:
	QSharedPointer<ImportDirectoryWindow> importDirectoryWindow;

    FourFlowMainWindow(const FourFlowMainWindow&); // Not implemented.
    void operator=(const FourFlowMainWindow&); // Not implemented.

};

#endif


