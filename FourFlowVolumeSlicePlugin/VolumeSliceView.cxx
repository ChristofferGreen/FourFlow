#include "VolumeSliceView.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QPixmap.h>
#include <QtGui/QPushButton>

#include <math.h>
#include <string>
#include <sstream>
using namespace std;

#include <pqPVApplicationCore.h>
#include <pqAnimationScene.h>
#include <pqAnimationManager.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMPropertyIterator.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkInformation.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkPVDataInformation.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkPVArrayInformation.h>
#include <vtkDataObject.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkSMProxyManager.h>
#include <vtkSMObject.h>
#include <vtkPointData.h>
#include <vtkSMProperty.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkPVCompositeRepresentation.h>
#include <vtkAlgorithmOutput.h>
#include <vtkSMRepresentationProxy.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMOutputPort.h>
#include "vtkSMCompoundSourceProxy.h"
#include "vtkAlgorithmOutput.h"
#include "vtkSICompoundSourceProxy.h"
#include "vtkCompositeDataProbeFilter.h"

#include "pqFiltersMenuReaction.h"
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqServer.h>
#include <pqDataRepresentation.h>
#include <pqScalarsToColors.h>
#include <pqPipelineRepresentation.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqServerManagerModel.h>
#include <pqSMAdaptor.h>
#include <pqSourcesMenuReaction.h>
#include <pqActiveObjects.h>
#include <pqDisplayPolicy.h>
#include <pqChangeInputDialog.h>
#include <pqCoreUtilities.h>


#include "VolumeSliceWidget.h"

/*
Hello,

Using a plugin I can create a point source with the following code:

  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  pqServerManagerModel* sm = core->getServerManagerModel();
  pqServerManagerSelectionModel* sms = core->getSelectionModel();
  pqServer *activeServer = sm->getItemAtIndex<pqServer*>(0);

  pqPipelineSource *newSource = builder->createSource("sources",
                "PointSource", activeServer);

I would like to add a vtkStringArray to the server side of this
source. From the server side,
I can easly do it using:
vtkPolyData::GetPointData()->AddArray("my vtkStringArray");

How can I do the same from the client side, using the pqPipelineSource
that I created ?

Thank you.
----
There's no easy way for doing that from the client side. Changing
output data objects outside of a filter/source is not recommended,
it's better to add those inside the filter/source's  RequestData().
Try creating a custom source instead.

Utkarsh
----
Hi,
Creating custom source will solve my problem.

Thanks Utkarsh.

*/

VolumeSliceView::VolumeSliceView(const QString& viewmoduletype, const QString& group, const QString& name, vtkSMViewProxy* viewmodule, pqServer* server, QObject* p) : pqView(viewmoduletype, group, name, viewmodule, server, p) {
	 this->volumeSliceWidget = new QWidget;
	 this->representation = 0;

	QPushButton *copyToNextButton = new QPushButton(this->volumeSliceWidget);
    copyToNextButton->setObjectName(QString::fromUtf8("copyToNextButton"));
    copyToNextButton->setLayoutDirection(Qt::LeftToRight);
    QIcon iconEmblemUnreadable;
    iconEmblemUnreadable.addFile(QString::fromUtf8(":/Icons/icons/copytonextframe.png"), QSize(), QIcon::Normal, QIcon::Off);
    copyToNextButton->setIcon(iconEmblemUnreadable);
    copyToNextButton->setIconSize(QSize(32, 32));
    copyToNextButton->setFlat(false);
	copyToNextButton->setGeometry(0, 0, 150, 40);
	copyToNextButton->setText("Copy To Next Frame");

	QPushButton *copyToAllButton = new QPushButton(this->volumeSliceWidget);
    copyToAllButton->setObjectName(QString::fromUtf8("copyToAllButton"));
    copyToAllButton->setLayoutDirection(Qt::LeftToRight);
    iconEmblemUnreadable.addFile(QString::fromUtf8(":/Icons/icons/copytoallframes.png"), QSize(), QIcon::Normal, QIcon::Off);
    copyToAllButton->setIcon(iconEmblemUnreadable);
    copyToAllButton->setIconSize(QSize(32, 32));
    copyToAllButton->setFlat(false);
	copyToAllButton->setGeometry(0, 550, 140, 40);
	copyToAllButton->setText("Copy To All Frames");

	this->intensitySlider = new QSlider(Qt::Orientation::Horizontal, this->volumeSliceWidget);
	intensitySlider->setGeometry(155, 1, 348, 20);
	intensitySlider->setRange(0, 400);
	intensitySlider->setValue(10);
	this->connect(this->intensitySlider, SIGNAL(valueChanged(int)), SLOT(sliderValueChanged(int)));

	QPushButton *deleteButton = new QPushButton(this->volumeSliceWidget);
    deleteButton->setObjectName(QString::fromUtf8("deleteButton"));
    deleteButton->setLayoutDirection(Qt::LeftToRight);
    iconEmblemUnreadable.addFile(QString::fromUtf8(":/Icons/icons/tango/32x32/emblems/emblem-unreadable.png"), QSize(), QIcon::Normal, QIcon::Off);
    deleteButton->setIcon(iconEmblemUnreadable);
    deleteButton->setIconSize(QSize(32, 32));
    deleteButton->setFlat(false);
	deleteButton->setGeometry(333, 550, 170, 40);
	deleteButton->setText("Delete Selected Vertices");

	sliceWidget = new VolumeSliceWidget(this->volumeSliceWidget);
	sliceWidget->setGeometry(2, 45, IMAGE_WIDTH, IMAGE_HEIGHT);
	sliceWidget->sliceView = this;

	// connect to display creation so we can show them in our view
	this->connect(this, SIGNAL(representationAdded(pqRepresentation*)), SLOT(onRepresentationAdded(pqRepresentation*)));
	this->connect(this, SIGNAL(representationRemoved(pqRepresentation*)), SLOT(onRepresentationRemoved(pqRepresentation*)));
	connect(copyToNextButton, SIGNAL(clicked()), this, SLOT(copyToNextButtonClicked()));
	connect(copyToAllButton, SIGNAL(clicked()), this, SLOT(copyToAllButtonClicked()));
	connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteButtonClicked()));
}

void VolumeSliceView::sliderValueChanged(int value) {
	this->updated();
}

VolumeSliceView::~VolumeSliceView() {
//todo: memleak
	/*delete representation;
	delete probeWindowUI;
	delete volumeSliceWidget;
	delete sliceWidget;*/
}

vtkSMProxy *VolumeSliceView::getVolume(){
	if(this->representation == 0)
		return 0;

	pqPipelineRepresentation *pipelineRep = (pqPipelineRepresentation*)this->representation;
	vtkSMRepresentationProxy *repProxy = pipelineRep->getRepresentationProxy();
	vtkSMPVRepresentationProxy *pvRepProxy = vtkSMPVRepresentationProxy::SafeDownCast(repProxy);

	vtkSMProxy *producerProxy = pvRepProxy->GetProducerProxy(0);
	vtkSMCompoundSourceProxy *smComp = vtkSMCompoundSourceProxy::SafeDownCast(producerProxy);
	vtkCompositeDataProbeFilter *probeFilter = 0;
	if(smComp) {
		int proxies = smComp->GetNumberOfProxies();
		vtkSMProxy *subProxy = 0;
		for(int i = 0; i < proxies; i++) {
			vtkSMProxy *tmpSubProxy = smComp->GetProxy((unsigned int)i);
			int producers = tmpSubProxy->GetNumberOfProducers();
			for(int i = 0; i < producers; i++) {
				vtkSMProxy *producerProxy = tmpSubProxy->GetProducerProxy(i);
				if(string(producerProxy->GetXMLName()).compare("ensight") == 0)
					return producerProxy;
			}
		}
	}
	return 0;
}

vtkPolyData *VolumeSliceView::getProbePlanePolyData() {
	if(this->representation == 0)
		return 0;
	pqPipelineRepresentation *pipelineRep = (pqPipelineRepresentation*)this->representation;
	vtkSMRepresentationProxy *repProxy = pipelineRep->getRepresentationProxy();
	vtkSMPVRepresentationProxy *pvRepProxy = vtkSMPVRepresentationProxy::SafeDownCast(repProxy);
	
	vtkSMProxy *producerProxy = pvRepProxy->GetProducerProxy(0);
	vtkSMCompoundSourceProxy *smComp = vtkSMCompoundSourceProxy::SafeDownCast(producerProxy);
	vtkCompositeDataProbeFilter *probeFilter = 0;
	if(smComp) {
	  int proxies = smComp->GetNumberOfProxies();
	  vtkSMProxy *subProxy = 0;
	  for(int i = 0; i < proxies; i++) {
		  vtkSMProxy *tmpSubProxy = smComp->GetProxy((unsigned int)i);
		  if(string(tmpSubProxy->GetXMLName()).compare("Probe Filter") == 0) {
			  subProxy = tmpSubProxy;
			  break;
		  }
	  }
	  if(subProxy) {
		  vtkObjectBase *clientObject = subProxy->GetClientSideObject();
		  probeFilter = vtkCompositeDataProbeFilter::SafeDownCast(clientObject);
	  }
	}
	if(!probeFilter)
		return 0;

	vtkDataSet *probeDataSet = probeFilter->GetOutput(0);
	vtkPolyData *polyData = vtkPolyData::SafeDownCast(probeDataSet);
	return polyData;
}

void VolumeSliceView::updatePlanePolyData() {
	if(this->representation == 0)
		return;
	pqPipelineRepresentation *pipelineRep = (pqPipelineRepresentation*)this->representation;
	vtkSMRepresentationProxy *repProxy = pipelineRep->getRepresentationProxy();
	vtkSMPVRepresentationProxy *pvRepProxy = vtkSMPVRepresentationProxy::SafeDownCast(repProxy);
	
	vtkSMProxy *producerProxy = pvRepProxy->GetProducerProxy(0);
	vtkSMCompoundSourceProxy *smComp = vtkSMCompoundSourceProxy::SafeDownCast(producerProxy);
	vtkCompositeDataProbeFilter *probeFilter = 0;
	if(smComp) {
	  int proxies = smComp->GetNumberOfProxies();
	  vtkSMProxy *subProxy = 0;
	  for(int i = 0; i < proxies; i++) {
		  vtkSMProxy *tmpSubProxy = smComp->GetProxy((unsigned int)i);
		  if(string(tmpSubProxy->GetXMLName()).compare("Probe Filter") == 0) {
			  subProxy = tmpSubProxy;
			  break;
		  }
	  }
	  if(subProxy) {
		  vtkObjectBase *clientObject = subProxy->GetClientSideObject();
		  probeFilter = vtkCompositeDataProbeFilter::SafeDownCast(clientObject);
	  }
	}
	if(!probeFilter)
		return;

	probeFilter->UpdateInformation();
	probeFilter->Update();
}

void VolumeSliceView::copyToAllButtonClicked() {
	for(int i = 0; i < this->numberOfTimeValues; i++) {
		if(i != sliceWidget->currentTimeIndex)
			sliceWidget->pointMap[i] = sliceWidget->pointMap[sliceWidget->currentTimeIndex];
	}
	this->updatePolygon();
}

void VolumeSliceView::copyToNextButtonClicked() {
	if(this->sliceWidget->currentTimeIndex < this->numberOfTimeValues-1) {
		sliceWidget->pointMap[sliceWidget->currentTimeIndex+1] = sliceWidget->pointMap[sliceWidget->currentTimeIndex];
		this->updatePolygon();
		pqAnimationManager *animationManager = pqPVApplicationCore::instance()->animationManager();
		pqAnimationScene *animationScene = animationManager->getActiveScene();
		animationScene->setCacheGeometrySetting(false);
		animationScene->getProxy()->InvokeCommand("GoToNext");
		animationScene->setCacheGeometrySetting(true);
	}
	else
		cout << "At final frame, should not copy" << endl;
}

void VolumeSliceView::updatePolygon() {
	//if(sliceWidget->pointMap[sliceWidget->currentTimeIndex].size() < 3) // bug change this
		//return;

	if(this->representation) {
		pqPipelineRepresentation *pipelineRep = (pqPipelineRepresentation*)this->representation;
		pqOutputPort *outPort = pipelineRep->getOutputPortFromInput();
		if(outPort->getNumberOfConsumers()) {
			vtkPolyData *polyData = this->getProbePlanePolyData();
			if(this->polygon && polyData) {
				if(vtkSMProxy *proxy = this->polygon->getProxy()) {
					string paramString;
					for(int i = 0; i < this->timeValues.size(); i++) {
						std::vector<QPoint> points = this->sliceWidget->pointMap[i];
						paramString = paramString + QString::number(this->timeValues[i]).toStdString()  + " ";
						foreach(QPoint point, points) {
							std::ostringstream s1, s2;
							s1 << ((double)point.x())/IMAGE_WIDTH;
							paramString = paramString + s1.str() + " ";
							s2 << ((double)point.y())/IMAGE_HEIGHT;
							paramString = paramString + s2.str() + " ";
						}
						paramString[paramString.size()-1]=':';
					}

					vtkSMStringVectorProperty *propString = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("Vertex2dString"));
					propString->SetNumberOfElements(1);
					propString->SetElement(0, paramString.c_str());
					proxy->UpdateProperty("Vertex2dString", 1);
					proxy->UpdateVTKObjects();
					pipelineRep->renderView(true);
					this->polygon->renderAllViews(true);
					if(pqView *view = this->representation->getView())
						view->forceRender();
				}
			}
		}
		else
			this->sliceWidget->pointMap.clear();
	}
}

void VolumeSliceView::polygonButtonClicked() {
	if(sliceWidget->pointMap[sliceWidget->currentTimeIndex].size() < 3) // bug change this
		return;
	pqActiveObjects* activeObjects = &pqActiveObjects::instance();
	pqApplicationCore *core = pqApplicationCore::instance();
	pqObjectBuilder *builder = core->getObjectBuilder();
	pqServerManagerModel *sm = core->getServerManagerModel();
	foreach(pqView *view, sm->findItems<pqView*>()) {
		if(view->getViewType() == "RenderView") {
			pqActiveObjects::instance().setActiveView(view);
			break;
		}
	}

	QMap<QString, QList<pqOutputPort*> > namedInputs;
	pqPipelineRepresentation *pipelineRep = (pqPipelineRepresentation*)this->representation;
	namedInputs["Input"].append(pipelineRep->getOutputPortFromInput());
	pqPipelineSource *newSource = builder->createFilter("hiddenfilters", "RingPolygonSource", namedInputs, activeObjects->activeServer());

	this->updatePolygon();
}

void VolumeSliceView::deleteButtonClicked()
{
	sliceWidget->deleteButtonClicked();
}

QWidget* VolumeSliceView::getWidget()
{
	return this->volumeSliceWidget;
}

void VolumeSliceView::render() {
	pqView::render();
	this->updated();
}

void VolumeSliceView::updated() {
	vtkPolyData *polyData = this->getProbePlanePolyData();
	//polyData->UpdateInformation(); //commented
	if(polyData) {
		this->setCurrentTime();
		/*vtkInformation *info = polyData->GetInformation();
		if(info) {
			double requestedTimeValue = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
			cout << "requestedTimeValue: " << requestedTimeValue << endl;
		}*/


		if(QString(polyData->GetClassName()) == "vtkPolyData") {
			int iQwidth = 101;
			int iQheight = 101;
			QImage siImage(iQwidth,iQheight,QImage::Format_Indexed8);
			siImage.setNumColors(256);

			QRgb value;
			for (int i = 0; i <= 255; i++) {
				value = qRgb(i,i,i);
				siImage.setColor(i,value);
			}
			unsigned char* ucSiImage;
			for (int i = 0; i < iQheight; i++) {
				ucSiImage = siImage.scanLine(i); // changes the content of siImage?
				for (int j = 0; j < iQwidth; j++) {
					int scale = (intensitySlider->value()/5.0)*255;
					int value = polyData->GetPointData()->GetScalars()->GetComponent((i * iQwidth) + j, 0)*scale;
					if(value > 255)
							value = 255;
					ucSiImage[j] = value;
				}
			}
			//QTransform transform;
			//transform.rotate(-90);
			//siImage = siImage.transformed(transform).mirrored(true, false);
			QPixmap siPixmap;
			siPixmap = QPixmap::fromImage(siImage,Qt::AutoColor);
			siPixmap = siPixmap.scaled(IMAGE_WIDTH,IMAGE_HEIGHT,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

			//VolumeSliceWidget *silabel = new VolumeSliceWidget;
			//silabel->setPixmap(siPixmap);
			sliceWidget->pixmap = siPixmap;
			sliceWidget->repaint();
			//this->VolumeSliceWidget->layout()->addWidget(silabel);
			//this->Labels.insert(representation, silabel);
			this->updatePlanePolyData();
		}
	}
}

void VolumeSliceView::setCurrentTime() {
	vtkSMProxy *volumeProxy = this->getVolume();
	if(volumeProxy != 0) {
		vtkSMSourceProxy *sourceproxy = vtkSMSourceProxy::SafeDownCast(volumeProxy);
		vtkSMProperty *timeStepProperty = sourceproxy->GetProperty("TimestepValues");
		vtkSMDoubleVectorProperty *doubleTimeStepProperty = vtkSMDoubleVectorProperty::SafeDownCast(timeStepProperty);
		int numberOfElements = doubleTimeStepProperty->GetNumberOfElements();
		this->numberOfTimeValues = numberOfElements;
		//cout << "number of timeStep elements: " << numberOfElements << endl;

		pqAnimationManager *animationManager = pqPVApplicationCore::instance()->animationManager();
		pqAnimationScene *animationScene = animationManager->getActiveScene();
		double currentTime = animationScene->getAnimationTime();
		//cout << "Current Time: " << currentTime << endl;

		int currentClosestValuePosition = 0;
		float currentClosestDistance = 1000000.0f;
		for(int i = 0; i < numberOfElements; i++) {
			float currentValue = doubleTimeStepProperty->GetElement(i);
			float distance = fabs(currentValue-currentTime);
			if(distance < currentClosestDistance) {
				currentClosestValuePosition = i;
				currentClosestDistance = distance;
			}
		}
		for(int i = 0; i < numberOfElements; i++)
			this->timeValues[i] = doubleTimeStepProperty->GetElement(i);

		//cout << "Current time index: " << currentClosestValuePosition << endl;
		this->sliceWidget->currentTimeIndex = currentClosestValuePosition;
		this->sliceWidget->selectedPoints.clear();
	}
}

void VolumeSliceView::onRepresentationAdded(pqRepresentation* rep) {
	cout << "VolumeSliceView::onRepresentationAdded" << endl;
	this->representation = rep;

	pqPipelineRepresentation *pipelineRep = (pqPipelineRepresentation*)this->representation;
	pqOutputPort *outPort = pipelineRep->getOutputPortFromInput();
	
	if(outPort->getNumberOfConsumers()) {
		this->polygon = outPort->getConsumer(0);
		vtkSMProperty *prop = this->polygon->getProxy()->GetProperty("Vertex2dString");
		vtkSMStringVectorProperty *stringProp = vtkSMStringVectorProperty::SafeDownCast(prop);
		QString propString(stringProp->GetElement(0));

		int currentTimeIndex = 0;
		QStringList vertices2dTimeStringList = propString.split(":");
		for(QStringList::iterator iter = vertices2dTimeStringList.begin(); iter != vertices2dTimeStringList.end(); iter++) {
			QStringList vertices2dStringList = (*iter).split(" ");
			float currentTime = vertices2dStringList.front().toFloat();
			vertices2dStringList.pop_front();
			for(QStringList::iterator iter2 = vertices2dStringList.begin(); iter2 != vertices2dStringList.end(); iter2++) {
				QPoint point;
				point.setX((*iter2).toFloat()*IMAGE_WIDTH);
				iter2++;
				point.setY((*iter2).toFloat()*IMAGE_HEIGHT);
				sliceWidget->pointMap[currentTimeIndex].push_back(point);
			}
			currentTimeIndex++;
		}
	}
	else {
		pqActiveObjects* activeObjects = &pqActiveObjects::instance();
		pqApplicationCore *core = pqApplicationCore::instance();
		pqObjectBuilder *builder = core->getObjectBuilder();
		pqServerManagerModel *sm = core->getServerManagerModel();

		foreach(pqView *view, sm->findItems<pqView*>()) {
			if(view->getViewType() == "RenderView") {
				pqActiveObjects::instance().setActiveView(view);
				break;
			}
		}

		QMap<QString, QList<pqOutputPort*> > namedInputs;
		namedInputs["Input"].append(pipelineRep->getOutputPortFromInput());
		this->polygon = builder->createFilter("hiddenfilters", "RingPolygonSource", namedInputs, activeObjects->activeServer());
		QObject::connect(this->polygon, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(polygonUpdated()));
	}
	//QObject::connect(this->representation, SIGNAL(dataUpdated()), this, SLOT(updated()));
	this->updated();
	this->updatePlanePolyData();
}

void VolumeSliceView::polygonUpdated() {
	if(sliceWidget->pointMap.size()==1) {
		for(int i = 0; i < this->numberOfTimeValues; i++) {
			sliceWidget->pointMap[i] = std::vector<QPoint>();
		}
		this->updatePolygon();		
	}
}

void VolumeSliceView::onRepresentationRemoved(pqRepresentation* d) {
	QObject::disconnect(d, SIGNAL(dataUpdated()), this, SLOT(updated()));
	this->representation = 0;
	this->sliceWidget->pointMap.clear();
	QPixmap siPixmap;
	this->sliceWidget->pixmap = siPixmap;
	this->sliceWidget->repaint();
	vtkSMProxy *proxy = getVolume();
}

bool VolumeSliceView::canDisplay(pqOutputPort* opPort) const {
	pqPipelineSource* source = opPort? opPort->getSource() : 0;
	// check valid source and server connections
	if(!source || this->getServer()->GetConnectionID() != source->getServer()->GetConnectionID()) {
		return false;
	}
	if(QString("ProbePlane") == source->getProxy()->GetXMLName()) {
		return true;
	}

	return false;
}


