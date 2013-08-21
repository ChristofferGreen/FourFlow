// FourFlow includes
#include "FourFlowMainWindow.h"
#include "pqInternals.h"
#include "pqIncludes.h"
#include "qtIncludes.h"
#include "vtkIncludes.h"
#include "cppIncludes.h"
#include "staticFunctions.h"
#include "vtkPVCompositeRepresentation.h"
#include "vtkDataObject.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSMCompoundSourceProxy.h"

FourFlowMainWindow::FourFlowMainWindow() : internals(new pqInternals()) {
	pqSettings* settings = pqApplicationCore::instance()->settings();
	settings->beginGroup("renderModule");
	settings->setValue("LODThreshold", VTK_DOUBLE_MAX);
	settings->endGroup();

	pqAnimationScene::setCacheLimitSetting(3024000);


	this->internals->fourFlowMainWindow = this;
	this->internals->setupUi(this);
	this->internals->setUpDefaults(this);
	this->internals->setUpAdvancedMenu();
	this->internals->addToolbars(this);
	this->internals->connectButtonSignalsWithSlots();

	QObject::connect(this->internals->actionOpenDir,		SIGNAL(triggered()), this, SLOT(importDir()));
	//new pqLoadDataReaction(this->internals->actionOpenDir);

	// Final step, define application behaviors. Since we want all ParaView
	// behaviors, we use this convenience method.

	this->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);

	this->removeDockWidget(this->internals->objectInspectorDock);
	this->removeDockWidget(this->internals->displayDock);
	delete this->internals->objectInspectorDock;
	delete this->internals->displayDock;
	this->internals->objectInspectorDock = 0;
	this->internals->displayDock = 0;
	this->showMaximized();

	this->tabifyDockWidget(this->internals->propertiesDock, this->internals->informationDock);
	this->internals->propertiesDock->show();
	this->internals->propertiesDock->raise();

	new pqParaViewBehaviors(this, this);

	pqApplicationCore *core = pqApplicationCore::instance();
	pqObjectBuilder *builder = core->getObjectBuilder();
	pqServerManagerModel *sm = core->getServerManagerModel();
	foreach(pqView *view, sm->findItems<pqView*>()) {
		if(view->getViewType() == "RenderView") {
			vtkSMPropertyHelper((vtkSMProxy*)view->getViewProxy(), "UseGradientBackground").Set(1);
			((vtkSMProxy*)view->getViewProxy())->UpdateVTKObjects();
			break;
		}
	}
}

FourFlowMainWindow::~FourFlowMainWindow() {
}

// Import the files from a dir into a window from where the user can select what files to import
void FourFlowMainWindow::importDir() {
	pqServer* server = pqActiveObjects::instance().activeServer();
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::Directory);
	QStringList fileNames;
	if(dialog.exec()) {
		fileNames = dialog.selectedFiles();

		std::queue<QDir> dirQueue; 
		dirQueue.push(QDir(fileNames.at(0)));

		this->importDirectoryWindow = QSharedPointer<ImportDirectoryWindow>(new ImportDirectoryWindow);
		this->importDirectoryWindow->ffWindow = this;
		this->importDirectoryWindow->show();

		// recursive dir search
		QDirIterator it(fileNames.at(0), QDirIterator::Subdirectories); 
		while (it.hasNext()) {
			QString path = it.next();
			QFileInfo fileInfo(path);
			if ( fileInfo.isFile() && fileInfo.suffix().compare("case") == 0 )
				this->importDirectoryWindow->internals->listWidget->addItem(path);
			if ( fileInfo.isFile() && fileInfo.suffix().compare("pvd") == 0 )
				this->importDirectoryWindow->internals->listWidget->addItem(path);
		}
	}
}

void FourFlowMainWindow::representationAddedSurface(pqPipelineSource *source, pqDataRepresentation *repr, int val) {
	QObject::disconnect(source, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedGreyscale(pqPipelineSource*, pqDataRepresentation*, int)));
	QObject::connect(repr, SIGNAL(dataUpdated()), this, SLOT(setRepresentationToSurface()));
}


/*vtkPolyData *VolumeSliceView::getProbePlanePolyData() {
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
}*/

void FourFlowMainWindow::setRepresentationToSurface() {
	pqDataRepresentation *repr = (pqDataRepresentation *)sender();
	QObject::disconnect(repr, SIGNAL(dataUpdated()), this, SLOT(setRepresentationToSurface()));
	pqSMAdaptor::setEnumerationProperty(repr->getProxy()->GetProperty("Representation"), "Surface");
	//pqSMAdaptor::setEnumerationProperty(repr->getProxy()->GetProperty("SAXPlane"), 10);
	//cout << "repr->getProxy()->GetProperty(\"SAXPlane\"): " << repr->get("SAXPlane") << endl;


	vtkSMPVRepresentationProxy *representationProxy = vtkSMPVRepresentationProxy::SafeDownCast(repr->getProxy());
	vtkSMProxy *producerProxy = representationProxy->GetProducerProxy(0);
	vtkSMSourceProxy *sourceProxy = vtkSMSourceProxy::SafeDownCast(producerProxy);
	pqSMAdaptor::setEnumerationProperty(sourceProxy->GetProperty("SAX plane"), 1);
	cout << "prop: " << sourceProxy->GetProperty("SAX plane") << endl;
	sourceProxy->UpdateProperty("SAX plane", true);
	sourceProxy->UpdateVTKObjects();
	vtkSMPropertyIterator *propIterator = sourceProxy->NewPropertyIterator();
	while(!propIterator->IsAtEnd()) {
		cout << "property label: " << propIterator->GetPropertyLabel() << endl;
		propIterator->Next();
	}



	/*vtkObjectBase *clientObject = repr->getProxy()->GetClientSideObject();
	vtkPVCompositeRepresentation *composition = vtkPVCompositeRepresentation::SafeDownCast(clientObject);
	vtkDataObject *dataObject = composition->GetRenderedDataObject(0);
	vtkMultiBlockDataSet *multiblockDataSet = vtkMultiBlockDataSet::SafeDownCast(dataObject);*/
	/*vtkSMPropertyIterator *propIterator = repr->getProxy()->NewPropertyIterator();
	while(!propIterator->IsAtEnd()) {
		cout << "property label: " << propIterator->GetPropertyLabel() << endl;
		propIterator->Next();
	}*/
}

void FourFlowMainWindow::representationAddedGreyscale(pqPipelineSource *source, pqDataRepresentation *repr, int val) {
	QObject::disconnect(source, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedGreyscale(pqPipelineSource*, pqDataRepresentation*, int)));
	QObject::connect(repr, SIGNAL(dataUpdated()), this, SLOT(setColorMapGreyscale()));
}

void FourFlowMainWindow::setColorMapGreyscale() {
	pqDataRepresentation *repr = (pqDataRepresentation *)sender();
	QObject::disconnect(repr, SIGNAL(dataUpdated()), this, SLOT(setColorMapGreyscale()));
	setColorMapGreyscale(repr);
}

void FourFlowMainWindow::setColorMapGreyscale(pqDataRepresentation *repr) {
	/*
		Change display type to polygon surface
	*/
	//pqSMAdaptor::setEnumerationProperty(repr->getProxy()->GetProperty("Representation"), "Surface");

	/*
		Create the lookup table
	*/
	QList<QVariant> rgbPoints;
	rgbPoints << 0.0 << 0.0 << 0.0 << 0.0;
	rgbPoints << 1.0 << 1.0 << 1.0 << 1.0;
	pqApplicationCore* core = pqApplicationCore::instance();
	pqObjectBuilder* builder = core->getObjectBuilder();
	vtkSMProxy *lookupTable = builder->createProxy("lookup_tables", "PVLookupTable", pqActiveObjects::instance().activeServer(), "lookup_tables");
	pqSMAdaptor::setMultipleElementProperty(lookupTable->GetProperty("RGBPoints"), rgbPoints);
	pqSMAdaptor::setEnumerationProperty(lookupTable->GetProperty("ColorSpace"), "RGB");

	/*
		Update proxy properties
	*/
	vtkSMStringVectorProperty *colorArrayName = vtkSMStringVectorProperty::SafeDownCast(repr->getProxy()->GetProperty("ColorArrayName"));
	if(string(colorArrayName->GetElement(0)).compare("Result") != 0)
		pqSMAdaptor::setEnumerationProperty(repr->getProxy()->GetProperty("ColorArrayName"), "MagnitudeP001N");
	pqSMAdaptor::setProxyProperty(repr->getProxy()->GetProperty("LookupTable"), lookupTable);
	pqSMAdaptor::setEnumerationProperty(repr->getProxy()->GetProperty("ColorAttributeType"), "POINT_DATA");

	lookupTable->UpdateVTKObjects();
	repr->getProxy()->UpdateVTKObjects();
	repr->renderViewEventually();
}