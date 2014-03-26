#include "ConnectionWindow.h"
#include "FourFlowMainWindow.h"
#include "staticFunctions.h"
#include "pqInternals.h"
#include "vtkIncludes.h"
#include <sstream>
#include <algorithm>
#include <iostream>

ConnectionWindow::ConnectionWindow() : internals(new Ui_ConnectionWindow) { 
	this->internals->setupUi(this); 
	connect(this->internals->nextButton, SIGNAL(clicked()), this, SLOT(nextClicked()));
	this->selectedInput=0;
	this->selectedSource=0;
}

void ConnectionWindow::show() {
	this->internals->title->setText(this->inputTitle);
	this->internals->text->setText(this->inputText);
	foreach(pqPipelineSource *source, this->inputs)
		this->internals->list->addItem(source->getSMName());
	if(!this->attemptToDeduceInputsAutomatically())
		QMainWindow::show();
}

bool ConnectionWindow::attemptToDeduceInputsAutomatically() {
	if(this->inputs.size()==1 && this->sources.size()==1) {
		this->selectedInput=this->inputs[0];
		this->selectedSource=this->sources[0];
		this->execute();
		return true;
	}
	return false;
}

void ConnectionWindow::execute() {
	pqApplicationCore *core = pqApplicationCore::instance();
	pqObjectBuilder *builder = core->getObjectBuilder();  
	QMap<QString, QList<pqOutputPort*> > namedInputs;
	namedInputs["Input"].append(this->selectedInput->getOutputPort(0));
	namedInputs["Source"].append(this->selectedSource->getOutputPort(0));
	pqPipelineSource *source;
	if(this->type==ParticleTrace)
		source = builder->createFilter("filters", "FourFlowParticleTraceSource", namedInputs, pqActiveObjects::instance().activeServer());
	else if(this->type==PathLines)
		source = builder->createFilter("filters", "CustomParticlePath", namedInputs, pqActiveObjects::instance().activeServer());
	else if(this->type==StreamLines)
		source = builder->createFilter("filters", "CustomStreamLines", namedInputs, pqActiveObjects::instance().activeServer());
	else if(this->type==ParticleCollector)
		source = builder->createFilter("filters", "FourFlowCollector", namedInputs, pqActiveObjects::instance().activeServer());
	else if(this->type==Graph)
		source = builder->createFilter("filters", "FourFlowGraph", namedInputs, pqActiveObjects::instance().activeServer());
	if(this->type==ParticleTrace||this->type==PathLines||this->type==StreamLines) {
		this->ffWindow->internals->preferedColorVariable = "V_3D_01";
		this->ffWindow->internals->useVelocityForColor(source);
	}
}

void ConnectionWindow::nextClicked() {
	if(this->internals->list->selectedItems().size()) {
		if(!this->selectedInput) {
			this->selectedInput=this->inputs[this->internals->list->currentRow()];
			this->internals->title->setText(this->sourceTitle);
			this->internals->text->setText(this->sourceText);
			this->internals->list->clear();
			this->internals->nextButton->setText("Create");
			foreach(pqPipelineSource *source, this->sources)
				this->internals->list->addItem(source->getSMName());
		}
		else if(!this->selectedSource) {
			this->selectedSource=this->sources[this->internals->list->currentRow()];
			this->execute();
			QMainWindow::hide();
		}
	}
}

void ConnectionWindow::setInputText(const QString &title, const QString &text) {
	this->inputTitle=title;
	this->inputText=text;
}

void ConnectionWindow::setSourceText(const QString &title, const QString &text) {
	this->sourceTitle=title;
	this->sourceText=text;
}

void ConnectionWindow::setType(Type type) {
	this->type=type;
}

bool IsVelocityVolume(pqPipelineSource *source) {
	if(!source)
		return false;
	if(source->getProxy()->GetXMLName() != QString("ensight"))
		return false;
	if(!source->getOutputPort(0))
		return false;
	if(!source->getOutputPort(0)->getDataInformation())
		return false;
	if(!source->getOutputPort(0)->getDataInformation()->GetPointDataInformation())
		return false;
	if(!source->getOutputPort(0)->getDataInformation()->GetPointDataInformation()->GetArrayInformation(0))
		return false;
	if(source->getOutputPort(0)->getDataInformation()->GetPointDataInformation()->GetArrayInformation(0)->GetNumberOfComponents() != 3)
		return false;
	return true;
}

bool IsRingPolygon(pqPipelineSource *source) {
	if(!source)
		return false;
	if(source->getProxy()->GetXMLName() == QString("RingPolygonSource"))
		return true;
	return false;
}

bool IsParticle(pqPipelineSource *source) {
	if(!source)
		return false;
	if(source->getProxy()->GetXMLName() == QString("FourFlowParticleTraceSource"))
		return true;
	return false;
}

void ConnectionWindow::fill() {
	pqApplicationCore *core = pqApplicationCore::instance();
	pqObjectBuilder *builder = core->getObjectBuilder();  
	pqServerManagerModel *sm = core->getServerManagerModel();
	pqActiveObjects* activeObjects = &pqActiveObjects::instance();
	QList<pqPipelineSource*> sources = sm->findItems<pqPipelineSource*>(activeObjects->activeServer());
	foreach(pqPipelineSource *source, sources) {
		if(IsVelocityVolume(source)) {
			if(this->type==ParticleTrace||this->type==PathLines||this->type==StreamLines||this->type==Graph)
				this->inputs.push_back(source);
		}
		if(IsParticle(source)) {
			if(this->type==ParticleCollector)
				this->inputs.push_back(source);
		}
		if(IsRingPolygon(source)) {
			if(this->type==ParticleTrace||this->type==PathLines||this->type==StreamLines||this->type==Graph||this->type==ParticleCollector)
				this->sources.push_back(source);
		}
	}
}

void ConnectionWindow::clear() {
	this->inputs.clear();
	this->sources.clear();
}