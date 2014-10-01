#include "pqInternals.h"
#include "FourFlowMainWindow.h"
#include "pqIncludes.h"
#include "vtkDataSet.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqActiveObjects.h"
#include "pqViewFrame.h"
#include "vtkSMViewProxy.h"
#include "staticFunctions.h"
#include "vtkSMDoubleVectorProperty.h"
#include "pqTimeKeeper.h"
#include "pqAutoApplyReaction.h"

pqInternals::pqInternals() {
	this->animationHidden = true;
	this->propertiesHidden = true;
	this->polygonEditorHidden = true;
}

void pqInternals::connectButtonSignalsWithSlots() {
	// Enable help for from the object inspector.
	QObject::connect(this->objectInspector, SIGNAL(helpRequested(QString)), this, SLOT(showHelpForProxy(const QString&)));

	this->actionProperties->toggle();
	this->actionAnimation->toggle();
	QObject::connect(this->actionAdvancedMenu,		SIGNAL(triggered()), this, SLOT(openAdvancedMenu()));
	QObject::connect(this->actionPolygonEditor,		SIGNAL(triggered()), this, SLOT(polygonEditorToggle()));
	QObject::connect(this->actionProbePlane,		SIGNAL(triggered()), this, SLOT(applyProbePlane()));
	QObject::connect(this->actionParticleTrace,		SIGNAL(triggered()), this, SLOT(applyParticleTrace()));
	QObject::connect(this->actionParticleCollector, SIGNAL(triggered()), this, SLOT(applyParticleCollector()));
	QObject::connect(this->actionStreamlines,		SIGNAL(triggered()), this, SLOT(applyStreamlines()));
	QObject::connect(this->actionGraph,				SIGNAL(triggered()), this, SLOT(applyGraph()));
	QObject::connect(this->actionPathlines,			SIGNAL(triggered()), this, SLOT(applyPathlines()));
	QObject::connect(this->actionVolume,			SIGNAL(triggered()), this, SLOT(applyVolume()));
	QObject::connect(this->actionVolumeTrace,		SIGNAL(triggered()), this, SLOT(applyVolumeTrace()));
	QObject::connect(this->actionClip,				SIGNAL(triggered()), this, SLOT(applyClip()));
	QObject::connect(this->actionIsoSurface,		SIGNAL(triggered()), this, SLOT(applyIsoSurface()));
	QObject::connect(this->actionUndo,				SIGNAL(triggered()), this, SLOT(onUndo()));
	QObject::connect(this->actionRedo,				SIGNAL(triggered()), this, SLOT(onRedo()));

	QObject::connect(&pqActiveObjects::instance(), SIGNAL(representationChanged(pqDataRepresentation*)), this, SLOT(updateEnableState()), Qt::QueuedConnection);

	this->updateEnableState();
}

void pqInternals::applyVolume() {
	setActiveRenderView();
	pqPipelineSource *source = pqFiltersMenuReaction::createFilter("filters", "VolumeResampler");
	if(source) {
		connect(source, SIGNAL(representationAdded(pqPipelineSource*,pqDataRepresentation*,int)), 
			SLOT(onRepresentationAddedVolume(pqPipelineSource*,pqDataRepresentation*,int)));
	}
}

void pqInternals::onRepresentationAddedVolume(pqPipelineSource *source, pqDataRepresentation *repr, int srcOutputPort) {
	connect(repr, SIGNAL(dataUpdated()), SLOT(dataUpdatedVolume()));
	disconnect(source, SIGNAL(representationAdded(pqPipelineSource*,pqDataRepresentation*,int)));
}

void pqInternals::dataUpdatedVolume() {
	pqDataRepresentation *repr = (pqDataRepresentation *)sender();
	((pqPipelineRepresentation*)repr)->setRepresentation("Volume");
	disconnect(repr, SIGNAL(dataUpdated()));
}

void pqInternals::applyIsoSurface() { // buggy
	setActiveRenderView();
	pqPipelineSource *source = pqActiveObjects::instance().activeSource(); 
	vtkSMProxy *proxy = source->getProxy();
	if(source->getOutputPort(0)->getDataInformation()->GetPointDataInformation()->GetArrayInformation(0)->GetNumberOfComponents() == 3) {
		pqFiltersMenuReaction::createFilter("filters", "IsoSurfaceCleanupCompound");
	}
	else {
		pqFiltersMenuReaction::createFilter("filters", "FourFlowIsoCleanupSurface");
	}
}

void pqInternals::applyClip() {
	setActiveRenderView();
	pqFiltersMenuReaction::createFilter("filters", "Clip");
}

void pqInternals::applyVolumeTrace() {
	setActiveRenderView();
	useVelocityForColor(pqFiltersMenuReaction::createFilter("filters", "FourFlowVolumeTrack"));
}

void pqInternals::applyGraph() {
	//autoConnectFilter("filters", "FourFlowGraph", false);
	this->fourFlowMainWindow->createConnectionWindow();
	this->fourFlowMainWindow->connectionWindow->setInputText("Flow Graph Input", "Please specify the velocity data volume to be used for the flow graph.");
	this->fourFlowMainWindow->connectionWindow->setSourceText("Flow Graph Source", "Please specify the polygon ring to be used for the flow graph.");
	this->fourFlowMainWindow->connectionWindow->setType(ConnectionWindow::Graph);
	this->fourFlowMainWindow->connectionWindow->fill();
	this->fourFlowMainWindow->showConnectionWindow();
}


void pqInternals::applyProbePlane()  {
	setActiveRenderView();
	pqPipelineSource *source = pqFiltersMenuReaction::createFilter("filters", "ProbePlane");
	QObject::connect(source, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this->fourFlowMainWindow, SLOT(representationAddedGreyscale(pqPipelineSource*, pqDataRepresentation*, int)));
	connect(source, SIGNAL(representationAdded(pqPipelineSource*,pqDataRepresentation*,int)), SLOT(onRepresentationAddedProbePlane(pqPipelineSource*,pqDataRepresentation*,int)));
}

void pqInternals::onRepresentationAddedProbePlane(pqPipelineSource*, pqDataRepresentation *rep, int) {
	QObject::connect(rep, SIGNAL(dataUpdated()), this, SLOT(probePlaneUpdated()));
}

void pqInternals::probePlaneUpdated() {
	updateVolumeSlice();
}

void pqInternals::onUndo() {
	pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
	if (!stack) {
		qCritical("No application wide undo stack.");
		return;
	}
	stack->undo();
}

void pqInternals::onRedo() {
	pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
	if (!stack) {
		qCritical("No application wide undo stack.");
		return;
	}
	stack->redo();
}

void pqInternals::updateEnableState() {
	cout << "pqInternals::updateEnableState()" << endl;
	pqPipelineRepresentation* repr = qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());

	this->actionPathlines->setEnabled(true);
	this->actionParticleTrace->setEnabled(true);
	this->actionStreamlines->setEnabled(true);

	this->actionProbePlane->setDisabled(true);
	this->actionVolume->setDisabled(true);
	this->actionIsoSurface->setDisabled(true);
	this->actionClip->setDisabled(true);
	this->actionVolumeTrace->setDisabled(true);
	this->actionGraph->setEnabled(true);
	if(repr) {
		//if(source->getOutputPort(0)->getDataInformation()->GetPointDataInformation()->GetArrayInformation(0)->GetNumberOfComponents() == 3) {
		pqPipelineSource *source = repr->getInput();
		if(source) {
			cout << "name: " << source->getProxy()->GetXMLName() << endl;
			bool hasThreeComponents = false;
			if(source->getOutputPort(0)->getDataInformation()->GetPointDataInformation()->GetArrayInformation(0)) {
				if(source->getOutputPort(0)->getDataInformation()->GetPointDataInformation()->GetArrayInformation(0)->GetNumberOfComponents() == 3)
					hasThreeComponents = true;
			}
			if(QString("fourFlowEnsight") == source->getProxy()->GetXMLName() ||
				QString("ensight") == source->getProxy()->GetXMLName()) {
				if(hasThreeComponents)
					this->actionProbePlane->setEnabled(true);
				this->actionVolume->setEnabled(true);
				this->actionIsoSurface->setEnabled(true);
				this->actionClip->setEnabled(true);
				if(source->getOutputPort(0)->getDataInformation()->GetPointDataInformation()->GetArrayInformation("Psi1"))
					this->actionVolumeTrace->setEnabled(true);
			}
			else if(QString("RingPolygonSource") == source->getProxy()->GetXMLName()
				|| QString("IsoSurface") == source->getProxy()->GetXMLName()
				|| QString("Contour") == source->getProxy()->GetXMLName()
				|| QString("Clip") == source->getProxy()->GetXMLName()
				|| QString("IsoSurface") == source->getProxy()->GetXMLName()) {
				this->actionParticleTrace->setEnabled(true);
				this->actionPathlines->setEnabled(true);
				this->actionStreamlines->setEnabled(true);
				this->actionGraph->setEnabled(true);
				this->actionClip->setEnabled(true);
			}
			else if(hasThreeComponents)
				this->actionClip->setEnabled(true);
		}
	}
}

pqPipelineSource *pqInternals::autoConnectFilter(string group, string filter, bool color) {
	pqServer* server = pqActiveObjects::instance().activeServer();
	pqApplicationCore *core = pqApplicationCore::instance();
	pqObjectBuilder *builder = core->getObjectBuilder();  
	//vtkSMProxyManager *pxm = vtkSMProxyManager::GetProxyManager();
	vtkSMSessionProxyManager* pxm = server->proxyManager();
	pqServerManagerModel *sm = core->getServerManagerModel();
	pqActiveObjects* activeObjects = &pqActiveObjects::instance();

	QMap<QString, QList<pqOutputPort*> > namedInputs;

    vtkSMProxy *filterProxy = pxm->GetPrototypeProxy(group.c_str(), filter.c_str());

	QList<pqPipelineSource*> sources = sm->findItems<pqPipelineSource*>(activeObjects->activeServer());
	int inputB = 0;
	int sourceB = 0;
	foreach(pqPipelineSource *source, sources) {
		if(QString("ensight") == source->getProxy()->GetXMLName() &&
			source->getOutputPort(0) &&
			source->getOutputPort(0)->getDataInformation() &&
			source->getOutputPort(0)->getDataInformation()->GetPointDataInformation()->GetArrayInformation(0) && 
			source->getOutputPort(0)->getDataInformation()->GetPointDataInformation()->GetArrayInformation(0)->GetNumberOfComponents() == 3) {
			namedInputs["Input"].append(source->getOutputPort(0));
			inputB++;
		}
		if(QString("RingPolygonSource") == source->getProxy()->GetXMLName()) {
			namedInputs["Source"].append(source->getOutputPort(0));
			sourceB++;
		}
	}
	if(sourceB != 1 || inputB != 1) {
		namedInputs.clear();
		pqChangeInputDialog dialog(filterProxy, pqCoreUtilities::mainWidget());
		dialog.setObjectName("SelectInputDialog");
		if(QDialog::Accepted != dialog.exec()) {
			return 0; 
		}
		namedInputs = dialog.selectedInputs();
	}
	pqPipelineSource *source = builder->createFilter(group.c_str(), filter.c_str(), namedInputs, server);
	if(color)
		useVelocityForColor(source);
	return source;
}

void pqInternals::useVelocityForColor(pqPipelineSource *source) {
	connect(source, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedColor(pqPipelineSource*, pqDataRepresentation*, int)));
}

void pqInternals::representationAddedColor(pqPipelineSource *source, pqDataRepresentation *repr, int val) {
	disconnect(source, SIGNAL(representationAdded(pqPipelineSource*, pqDataRepresentation*, int)), this, SLOT(representationAddedColor(pqPipelineSource*, pqDataRepresentation*, int)));
	connect(repr, SIGNAL(dataUpdated()), this, SLOT(updatedColor()));
}

void pqInternals::updatedColor() {
	pqDataRepresentation *repr = (pqDataRepresentation *)sender();
	int arrays = repr->getInputDataInformation()->GetPointDataInformation()->GetNumberOfArrays();
	char *selectedArray = 0;
	for(int i = 0; i < arrays; i++) {
		char *currentArray = repr->getInputDataInformation()->GetPointDataInformation()->GetArrayInformation(i)->GetName();
		if(string(currentArray).compare(preferedColorVariable)==0) {
			selectedArray = currentArray;
			break;
		}
	}
	if(selectedArray == 0)
		return;
	disconnect(repr, SIGNAL(dataUpdated()), this, SLOT(updatedColor()));
	useColorVariable(repr);
}

void pqInternals::useColorVariable(pqDataRepresentation *repr) {
	int arrays = repr->getInputDataInformation()->GetPointDataInformation()->GetNumberOfArrays();
	char *selectedArray = 0;
	for(int i = 0; i < arrays; i++) {
		char *currentArray = repr->getInputDataInformation()->GetPointDataInformation()->GetArrayInformation(i)->GetName();
		if(string(currentArray).compare(preferedColorVariable)==0) {
			selectedArray = currentArray;
			break;
		}
	}
	if(!selectedArray) {
		selectedArray = "";
		for(int i = 0; i < arrays; i++) {
			char *currentArray = repr->getInputDataInformation()->GetPointDataInformation()->GetArrayInformation(i)->GetName();
			if(string(currentArray).compare("V_3D_01")==0) {
				selectedArray = currentArray;
				break;
			}
			if(string(currentArray).compare("FTLE")==0) {
				selectedArray = currentArray;
				break;
			}
		}
	}
	((pqPipelineRepresentation*)repr)->colorByArray(selectedArray, 0);
	this->fourFlowMainWindow->setColorMapRed(repr);
	repr->getProxy()->UpdateVTKObjects();
	repr->renderView(true);
}

void pqInternals::applyParticleCollector() {
	setActiveRenderView();
	this->fourFlowMainWindow->createConnectionWindow();
	this->fourFlowMainWindow->connectionWindow->setInputText("Particle Collector Input", "Please specify the particle trace simulation to be used for the particle collection.");
	this->fourFlowMainWindow->connectionWindow->setSourceText("Particle Collector Source", "Please specify the polygon ring that will collect the particles.");
	this->fourFlowMainWindow->connectionWindow->setType(ConnectionWindow::ParticleCollector);
	this->fourFlowMainWindow->connectionWindow->fill();
	this->fourFlowMainWindow->showConnectionWindow();
	/*setActiveRenderView();
	pqPipelineSource *source = autoConnectFilter("filters", "FourFlowCollector", true);*/
}

void pqInternals::applyParticleTrace() {
	setActiveRenderView();
	this->fourFlowMainWindow->createConnectionWindow();
	this->fourFlowMainWindow->connectionWindow->setInputText("Particle Trace Input", "Please specify the velocity data volume to be used for the simulation.");
	this->fourFlowMainWindow->connectionWindow->setSourceText("Particle Trace Source", "Please specify the polygon ring to be used for the simulation.");
	this->fourFlowMainWindow->connectionWindow->setType(ConnectionWindow::ParticleTrace);
	this->fourFlowMainWindow->connectionWindow->fill();
	this->fourFlowMainWindow->showConnectionWindow();
	//this->fourFlowMainWindow->fillVelocityVolumeNodes();
	/*setActiveRenderView();
	this->preferedColorVariable = "V_3D_01";
	//pqPipelineSource *source = autoConnectFilter("FourFlowParticleTracerCompound", true);
	//pqPipelineSource *source = autoConnectFilter("ParticleTracer", true);
	pqPipelineSource *source = autoConnectFilter("particles", "FourFlowParticleTraceSource", true);
	
	if(!source)
		return;
	useVelocityForColor(source);

	vtkSMProxy *sourceProxy = source->getProxy();

	pqAnimationManager *animationManager = pqPVApplicationCore::instance()->animationManager();
	pqAnimationScene *animationScene = animationManager->getActiveScene();
	pqTimeKeeper *timekeeper = animationScene->getServer()->getTimeKeeper();
	double currentTime = timekeeper->getTime();
	int currentFrame = timekeeper->getTimeStepValueIndex(currentTime);

	vtkSMProperty *prop = sourceProxy->GetProperty("ParticleReleaseStartFrame");
	std::cout << "current frame: " << currentFrame << " " << prop << std::endl;
	pqSMAdaptor::setElementProperty(prop, currentFrame);
	sourceProxy->UpdateProperty("ParticleReleaseStartFrame", true);
	sourceProxy->UpdateVTKObjects();*/
}

void pqInternals::applyStreamlines() {
	setActiveRenderView();
	this->fourFlowMainWindow->createConnectionWindow();
	this->fourFlowMainWindow->connectionWindow->setInputText("Stream Lines Input", "Please specify the velocity data volume to be used for the simulation.");
	this->fourFlowMainWindow->connectionWindow->setSourceText("Stream Lines Source", "Please specify the polygon ring to be used for the simulation.");
	this->fourFlowMainWindow->connectionWindow->setType(ConnectionWindow::StreamLines);
	this->fourFlowMainWindow->connectionWindow->fill();
	this->fourFlowMainWindow->showConnectionWindow();
/*	setActiveRenderView();
	preferedColorVariable = "V_3D_01";
	autoConnectFilter("filters", "CustomStreamLines", true);*/
}

void pqInternals::applyPathlines() {
	setActiveRenderView();
	this->fourFlowMainWindow->createConnectionWindow();
	this->fourFlowMainWindow->connectionWindow->setInputText("Path Line Input", "Please specify the velocity data volume to be used for the simulation.");
	this->fourFlowMainWindow->connectionWindow->setSourceText("Path line Source", "Please specify the polygon ring to be used for the simulation.");
	this->fourFlowMainWindow->connectionWindow->setType(ConnectionWindow::PathLines);
	this->fourFlowMainWindow->connectionWindow->fill();
	this->fourFlowMainWindow->showConnectionWindow();
	/*setActiveRenderView();
	preferedColorVariable = "V_3D_01";
	autoConnectFilter("filters", "CustomParticlePath", true);*/
	/*setActiveRenderView();
	preferedColorVariable = "V_3D_01";
	pqPipelineSource *source = pqFiltersMenuReaction::createFilter("filters", "ParticlePath");
	useVelocityForColor(source);
	vtkSMProxy *sourceProxy = source->getProxy();
	vtkSMProperty *prop = sourceProxy->GetProperty("Id Channel Array");
	pqSMAdaptor::setElementProperty(prop, "ParticleId");
	sourceProxy->UpdateProperty("Id Channel Array", true);
	sourceProxy->UpdateVTKObjects();*/
}

// Enable sane clicking behaviour for the advanced menu button (ie it should open right away)
void pqInternals::openAdvancedMenu() {
	QList<QWidget *> widgetList = this->actionAdvancedMenu->associatedWidgets();
	if(widgetList.size()) {
		QWidget *widget = widgetList.back();
		QMenu *menu = this->actionAdvancedMenu->menu();
		menu->popup(widgetList.front()->mapToGlobal(widget->geometry().bottomLeft()));
	}
}

void pqInternals::showHelpForProxy(const QString& proxyname) {
	pqHelpReaction::showHelp(QString("qthelp://paraview.org/paraview/%1.html").arg(proxyname));
}

void pqInternals::addToolbars(FourFlowMainWindow *mainWindow) {
	mainWindow->addToolBarBreak();
	QToolBar* vcrToolbar = new pqVCRToolbar(mainWindow) << pqSetName("VCRToolbar");
	vcrToolbar->layout()->setSpacing(0);
	mainWindow->addToolBar(Qt::TopToolBarArea, vcrToolbar);

	QToolBar* timeToolbar = new pqAnimationTimeToolbar(mainWindow) << pqSetName("currentTimeToolbar");
	timeToolbar->layout()->setSpacing(0);
	mainWindow->addToolBar(Qt::TopToolBarArea, timeToolbar);

/*	QToolBar* selectionToolbar = new pqSelectionToolbar(mainWindow) << pqSetName("selectionToolbar");
	selectionToolbar->layout()->setSpacing(0);
	mainWindow->addToolBar(Qt::TopToolBarArea, selectionToolbar);*/

	QToolBar* cameraToolbar = new pqCameraToolbar(mainWindow) << pqSetName("cameraToolbar");
	cameraToolbar->layout()->setSpacing(0);
	mainWindow->addToolBar(Qt::TopToolBarArea, cameraToolbar);

	QToolBar* axesToolbar = new pqAxesToolbar(mainWindow) << pqSetName("axesToolbar");
	axesToolbar->layout()->setSpacing(0);
	mainWindow->addToolBar(Qt::TopToolBarArea, axesToolbar);
}

void pqInternals::setUpDefaults(FourFlowMainWindow *mainWindow) {
	// Setup default GUI layout.

	// Set up the dock window corners to give the vertical docks more room.
	mainWindow->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	mainWindow->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

	// Enable automatic creation of representation on accept.
	pqAutoApplyReaction::setAutoApply(true);

	// setup the context menu for the pipeline browser.
	pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(*this->pipelineBrowser);

	new pqQtMessageHandlerBehavior(this);
	new pqLoadStateReaction(this->actionLoadState);
	new pqSaveStateReaction(this->actionSaveState);
	new pqPythonShellReaction(this->actionScripting << pqSetName("actionToolsPythonShell"));
	new pqServerDisconnectReaction(this->actionNewScene);

	// set default background to gradient
	/*pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqRenderView*> views = smModel->findItems<pqRenderView*>();
    foreach (pqRenderView* view, views) {
		vtkSMProxy* proxy = view->getProxy();
		std::cout << "UseGradientBackground " << proxy << std::endl;
		vtkSMPropertyHelper(proxy, "UseGradientBackground").Set(1);
	}
	std::cout << "end" << std::endl;*/
}


void pqInternals::polygonEditorToggle() {
	pqApplicationCore *core = pqApplicationCore::instance();
	pqServer *server=core->getActiveServer();
	pqObjectBuilder *builder = core->getObjectBuilder();

	pqPipelineRepresentation *repr = qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
	this->polygonEditorView = builder->createView("VolumeSliceView", server);
	if(repr) {
		pqDisplayPolicy *display_policy = pqApplicationCore::instance()->getDisplayPolicy();
		display_policy->setRepresentationVisibility(repr->getInput()->getOutputPort(0), this->polygonEditorView, true);
	}
}

void pqInternals::polygonEditorFrameClosed() {
	if(polygonEditorHidden) {
	}
	else {
		this->actionPolygonEditor->setChecked(false);
		this->polygonEditorView = 0;
	}
	polygonEditorHidden = !polygonEditorHidden;
}

void pqInternals::setUpAdvancedMenu() {
	QAction *saveScreenshotAction = advancedMenu.addAction("Save Screenshot");
	new pqSaveScreenshotReaction(saveScreenshotAction);
	QAction *saveAnimationAction = advancedMenu.addAction("Save Animation");
	new pqSaveAnimationReaction(saveAnimationAction);
	QAction *saveDataAction = advancedMenu.addAction("Save Data");
	new pqSaveDataReaction(saveDataAction);
	QAction *applicationSettingsAction = advancedMenu.addAction("Application Settings");
	new pqApplicationSettingsReaction(applicationSettingsAction);
	QAction *managePluginsAction = advancedMenu.addAction("Manage Plugins");
	new pqManagePluginsReaction(managePluginsAction);
	QAction *manageFiltersAction = advancedMenu.addAction("Manage Custom Filters");
	new pqManageCustomFiltersReaction(manageFiltersAction);
	pqParaViewMenuBuilders::buildFiltersMenu(*this->advancedMenu.addMenu("Filters"));
	pqParaViewMenuBuilders::buildSourcesMenu(*this->advancedMenu.addMenu("Sources"));
	this->actionAdvancedMenu->setMenu(&this->advancedMenu);
}
