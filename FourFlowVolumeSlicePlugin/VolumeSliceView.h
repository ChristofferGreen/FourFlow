#ifndef _VolumeSliceView_h
#define _VolumeSliceView_h

#include <QMap>
#include <QColor>
#include <QSlider>

#include <list>

#include "pqView.h"

#include "ui_probeResultWindow.h"
class VolumeSliceWidget;
class vtkPolyData;
class QLabel;
class pqRepresentation;

class VolumeSliceView : public pqView
{
	Q_OBJECT
public:
	/// constructor takes a bunch of init stuff and must have this signature to 
	/// satisfy pqView
	VolumeSliceView(const QString& viewtypemodule, const QString& group, const QString& name, 
		 vtkSMViewProxy* viewmodule, pqServer* server, QObject* p);
	~VolumeSliceView();

	/// don't support save images
	bool saveImage(int, int, const QString& ) { return false; }
	vtkImageData* captureImage(int) { return NULL; }
	vtkImageData* captureImage(const QSize&) { return NULL; }

	/// return the QWidget to give to ParaView's view manager
	QWidget* getWidget();

	/// returns whether this view can display the given source
	bool canDisplay(pqOutputPort* opPort) const;

	void updatePolygon();

protected slots:
	void onRepresentationAdded(pqRepresentation*);
	void onRepresentationRemoved(pqRepresentation*);
	void updated();
	void deleteButtonClicked();
	void polygonButtonClicked();
	void sliderValueChanged(int value);
	void copyToAllButtonClicked();
	void copyToNextButtonClicked();

protected:
	pqRepresentation *representation;
	pqPipelineSource *polygon;
	Ui_probeResultsWindow *probeWindowUI;
	QWidget* volumeSliceWidget;
	QMap<pqRepresentation*, VolumeSliceWidget*> Labels;
	QMap<int, float> timeValues;
	VolumeSliceWidget *sliceWidget;
	int numberOfTimeValues;
	QSlider *intensitySlider;

	void convertCoordinates(int px, int py, double &x, double &y, double &z);
	vtkPolyData *getProbePlanePolyData();
	vtkSMProxy *getVolume();
	void setCurrentTime();
};

#endif // _VolumeSliceView_h

