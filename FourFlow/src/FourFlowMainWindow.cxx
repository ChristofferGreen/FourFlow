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
#include <QShortcut>
#include <QCloseEvent>
#include <QMessageBox>

void FourFlowMainWindow::closeEvent(QCloseEvent *event) {
	QMessageBox::StandardButton resBtn = QMessageBox::question( this, "FourFlow", tr("Save before exit?\n"), QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
	if (resBtn == QMessageBox::No)
		event->accept();
	else if (resBtn == QMessageBox::Cancel)
		event->ignore();
	else if (resBtn == QMessageBox::Yes) {
		pqSaveStateReaction::saveState();
		event->accept();
	}
}

void FourFlowMainWindow::nextTimeStepShortCut() {
	pqAnimationManager *animationManager = pqPVApplicationCore::instance()->animationManager();
	pqAnimationScene *animationScene = animationManager->getActiveScene();
	animationScene->setCacheGeometrySetting(false);
	animationScene->getProxy()->InvokeCommand("GoToNext");
	animationScene->setCacheGeometrySetting(true);
}

void FourFlowMainWindow::previousTimeStepShortCut() {
	pqAnimationManager *animationManager = pqPVApplicationCore::instance()->animationManager();
	pqAnimationScene *animationScene = animationManager->getActiveScene();
	animationScene->setCacheGeometrySetting(false);
	animationScene->getProxy()->InvokeCommand("GoToPrevious");
	animationScene->setCacheGeometrySetting(true);
}

void FourFlowMainWindow::saxUpShortCut() {
	pqServer* server = pqActiveObjects::instance().activeServer();
	pqApplicationCore *core = pqApplicationCore::instance();
	pqObjectBuilder *builder = core->getObjectBuilder();  
	pqServerManagerModel *sm = core->getServerManagerModel();
	pqActiveObjects* activeObjects = &pqActiveObjects::instance();

	QList<pqPipelineSource*> sources = sm->findItems<pqPipelineSource*>(activeObjects->activeServer());
	foreach(pqPipelineSource *source, sources) {
		if(QString("SAX") == source->getProxy()->GetXMLName()) {
			pqSMAdaptor::setElementProperty(source->getProxy()->GetProperty("SAX plane"), pqSMAdaptor::getElementProperty(source->getProxy()->GetProperty("SAX plane")).toInt()+1);
			source->getProxy()->UpdateProperty("SAX plane", true);
			source->getProxy()->UpdateVTKObjects();
			source->renderAllViews(true);
		}
	}
}

void FourFlowMainWindow::saxDownShortCut() {
	pqServer* server = pqActiveObjects::instance().activeServer();
	pqApplicationCore *core = pqApplicationCore::instance();
	pqObjectBuilder *builder = core->getObjectBuilder();  
	pqServerManagerModel *sm = core->getServerManagerModel();
	pqActiveObjects* activeObjects = &pqActiveObjects::instance();

	QList<pqPipelineSource*> sources = sm->findItems<pqPipelineSource*>(activeObjects->activeServer());
	foreach(pqPipelineSource *source, sources) {
		if(QString("SAX") == source->getProxy()->GetXMLName()) {
			pqSMAdaptor::setElementProperty(source->getProxy()->GetProperty("SAX plane"), pqSMAdaptor::getElementProperty(source->getProxy()->GetProperty("SAX plane")).toInt()-1);
			source->getProxy()->UpdateProperty("SAX plane", true);
			source->getProxy()->UpdateVTKObjects();
			source->renderAllViews(true);
		}
	}
}


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

	  /// Provide access to the color-editor panel for the application.
	pqApplicationCore::instance()->registerManager("COLOR_EDITOR_PANEL", this->internals->colorMapEditorDock);


	QObject::connect(this->internals->actionOpenDir,		SIGNAL(triggered()), this, SLOT(importDir()));

	QShortcut *leftArrowShortCut = new QShortcut(Qt::Key_Left, this);
	leftArrowShortCut->setContext(Qt::ApplicationShortcut);
	QObject::connect(leftArrowShortCut, SIGNAL(activated()), this, SLOT(previousTimeStepShortCut()));

	QShortcut *rightArrowShortCut = new QShortcut(Qt::Key_Right, this);
	rightArrowShortCut->setContext(Qt::ApplicationShortcut);
	QObject::connect(rightArrowShortCut, SIGNAL(activated()), this, SLOT(nextTimeStepShortCut()));

	QShortcut *upArrowShortCut = new QShortcut(Qt::Key_Up, this);
	upArrowShortCut->setContext(Qt::ApplicationShortcut);
	QObject::connect(upArrowShortCut, SIGNAL(activated()), this, SLOT(saxUpShortCut()));

	QShortcut *downArrowShortCut = new QShortcut(Qt::Key_Down, this);
	downArrowShortCut->setContext(Qt::ApplicationShortcut);
	QObject::connect(downArrowShortCut, SIGNAL(activated()), this, SLOT(saxDownShortCut()));

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

//	this->showMaximized();
	setWindowState(Qt::WindowMaximized);
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

void FourFlowMainWindow::showConnectionWindow() {
	this->connectionWindow->show();
}

void FourFlowMainWindow::createConnectionWindow() {
	this->connectionWindow = QSharedPointer<ConnectionWindow>(new ConnectionWindow);
	this->connectionWindow->ffWindow = this;
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
	pqApplicationCore* core = pqApplicationCore::instance();
	pqObjectBuilder* builder = core->getObjectBuilder();
	
	//repr->getProxy()->GetProperty("LookupTable")->PrintSelf(std::cout, vtkIndent(0));

	vtkSMProxy *newLookupTable = builder->createProxy("lookup_tables", "PVLookupTable", pqActiveObjects::instance().activeServer(), "lookup_tables");
	pqSMAdaptor::setProxyProperty(repr->getProxy()->GetProperty("LookupTable"), newLookupTable);
	repr->getProxy()->UpdateVTKObjects();

	vtkSMProxy *newOpacityFunction = builder->createProxy("piecewise_functions", "PiecewiseFunction", pqActiveObjects::instance().activeServer(), "piecewise_functions");
    pqSMAdaptor::setProxyProperty(repr->getProxy()->GetProperty("ScalarOpacityFunction"), newOpacityFunction);
	vtkSMPropertyHelper(newLookupTable, "ScalarOpacityFunction").Set(newOpacityFunction);
	newLookupTable->UpdateVTKObjects();
	repr->getProxy()->UpdateVTKObjects();

	if(vtkSMProxy *lookupTableProxy = repr->getLookupTableProxy()) {
		QList<QVariant> rgbPoints;
		rgbPoints << 0.0 << 0.0 << 0.0 << 0.0;
		rgbPoints << 0.2 << 1.0 << 1.0 << 1.0;
		rgbPoints << 1.0 << 1.0 << 1.0 << 1.0;
		
		pqSMAdaptor::setMultipleElementProperty(lookupTableProxy->GetProperty("RGBPoints"), rgbPoints);
		pqSMAdaptor::setEnumerationProperty(lookupTableProxy->GetProperty("ColorSpace"), "RGB");
		pqSMAdaptor::setEnumerationProperty(lookupTableProxy->GetProperty("VectorMode"), "Magnitude");
		repr->getProxy()->UpdateVTKObjects();

		if(vtkSMProxy *opacityFunctionProxy =  repr->getScalarOpacityFunctionProxy()) {
			QList<QVariant> opacityValues;
			opacityValues << 0.0  << 0.0 << 0.5 << 0.0 ;
			opacityValues << 0.04 << 1.0 << 0.5 << 0.0 ;
			opacityValues << 1.0  << 1.0 << 0.5 << 0.0 ;

			pqSMAdaptor::setMultipleElementProperty(opacityFunctionProxy->GetProperty("Points"), opacityValues);
			repr->getProxy()->UpdateVTKObjects();
		}
	}
	repr->renderViewEventually();


	/*//Change display type to polygon surface
	//pqSMAdaptor::setEnumerationProperty(repr->getProxy()->GetProperty("Representation"), "Surface");

	//Create the lookup table
	QList<QVariant> rgbPoints;
	rgbPoints << 0.0 << 0.0 << 0.0 << 0.0;
	rgbPoints << 0.2 << 1.0 << 1.0 << 1.0;
	rgbPoints << 1.0 << 1.0 << 1.0 << 1.0;
	pqApplicationCore* core = pqApplicationCore::instance();
	pqObjectBuilder* builder = core->getObjectBuilder();
	vtkSMProxy *lookupTable = builder->createProxy("lookup_tables", "PVLookupTable", pqActiveObjects::instance().activeServer(), "lookup_tables");
	vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(repr->getProxy()->GetProperty("ScalarOpacityFunction"));
	if(pp) {
		std::cout << "Setting ScalarOpacityFunction property!" << std::endl;
		pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
		vtkSMProxy *opacityFunction = builder->createProxy("piecewise_functions", "PiecewiseFunction", pqActiveObjects::instance().activeServer(), "piecewise_functions");
		// Setup default opacity function to go from (0.0,0.0) to (1.0,1.0).
		// We are new setting defaults for midPoint (0.5) and sharpness(0.0) 
		QList<QVariant> values;
		values << 0.0 << 0.0 << 0.5 << 0.0 ;
		values << 1.0 << 1.0 << 0.5 << 0.0 ;
	    
		pqSMAdaptor::setMultipleElementProperty(opacityFunction->GetProperty("Points"), values);
		opacityFunction->UpdateVTKObjects();
		pqSMAdaptor::setProxyProperty(pp, opacityFunction);
		repr->getProxy()->UpdateVTKObjects();
	}
	else
		std::cout << "----- Could not find ScalarOpacityFunction property!" << std::endl;
	pqSMAdaptor::setMultipleElementProperty(lookupTable->GetProperty("RGBPoints"), rgbPoints);
	pqSMAdaptor::setEnumerationProperty(lookupTable->GetProperty("ColorSpace"), "RGB");

	//Update proxy properties
	vtkSMStringVectorProperty *colorArrayName = vtkSMStringVectorProperty::SafeDownCast(repr->getProxy()->GetProperty("ColorArrayName"));
	if(string(colorArrayName->GetElement(0)).compare("Result") != 0)
		pqSMAdaptor::setEnumerationProperty(repr->getProxy()->GetProperty("ColorArrayName"), "MagnitudeP001N");
	pqSMAdaptor::setProxyProperty(repr->getProxy()->GetProperty("LookupTable"), lookupTable);
	pqSMAdaptor::setEnumerationProperty(repr->getProxy()->GetProperty("ColorAttributeType"), "POINT_DATA");


	lookupTable->UpdateVTKObjects();
	repr->getProxy()->UpdateVTKObjects();
	repr->renderViewEventually();*/
}

void FourFlowMainWindow::setColorMapRed(pqDataRepresentation *repr) {
	QList<QVariant> rgbPoints;
	rgbPoints << 0.0 << 0.0 << 0.0 << 0.0;
	rgbPoints << 0.28 << 1.0 << 0.0 << 0.0;
	rgbPoints << 0.56 << 1.0 << 1.0 << 0.0;
	rgbPoints << 1.0 << 1.0 << 1.0 << 1.0;
	pqApplicationCore* core = pqApplicationCore::instance();
	pqObjectBuilder* builder = core->getObjectBuilder();
	vtkSMProxy *lookupTable = builder->createProxy("lookup_tables", "PVLookupTable", pqActiveObjects::instance().activeServer(), "lookup_tables");
	pqSMAdaptor::setMultipleElementProperty(lookupTable->GetProperty("RGBPoints"), rgbPoints);
	pqSMAdaptor::setEnumerationProperty(lookupTable->GetProperty("ColorSpace"), "RGB");
	pqSMAdaptor::setEnumerationProperty(lookupTable->GetProperty("VectorMode"), "Magnitude");
	pqSMAdaptor::setProxyProperty(repr->getProxy()->GetProperty("LookupTable"), lookupTable);

	lookupTable->UpdateVTKObjects();
	repr->getProxy()->UpdateVTKObjects();
	repr->renderViewEventually();
}