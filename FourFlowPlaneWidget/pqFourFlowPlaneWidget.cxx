#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqFourFlowPlaneWidget.h"
#include "pqPropertyLinks.h"
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "vtkEventQtSlotConnect.h"

#include <QDoubleValidator>

#include <vtkMath.h>
#include <vtkPVDataInformation.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMNewWidgetRepresentationProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSourceProxy.h>

class pqFourFlowPlaneWidget::pqImplementation {
public:
  pqImplementation() {}
  ~pqImplementation() {}
  
  /// Maps Qt widgets to the 3D widget
  pqPropertyLinks Links;
};

pqFourFlowPlaneWidget::pqFourFlowPlaneWidget(vtkSMProxy* o, vtkSMProxy* pxy, QWidget* p, const char* xmlname) : Superclass(o, pxy, p), Implementation(new pqImplementation()) {
	std::cout<<"pqFourFlowPlaneWidget::pqFourFlowPlaneWidget"<<std::endl;  
  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  
  this->createWidget(smmodel->findServer(o->GetSession()), xmlname);
  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()), this, SLOT(setModified()));
}

pqFourFlowPlaneWidget::~pqFourFlowPlaneWidget() {
	std::cout<<"pqFourFlowPlaneWidget::~pqFourFlowPlaneWidget"<<std::endl;
  if(vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy()) {
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
      
    this->setWidgetProxy(0);
  }

  delete this->Implementation;
}

void pqFourFlowPlaneWidget::pick(double dx, double dy, double dz) {
	std::cout<<"pqFourFlowPlaneWidget::pick"<<std::endl;
  vtkSMProxy* widget = this->getWidgetProxy();
  QList<QVariant> value;
  value << dx << dy << dz;
  widget->UpdateVTKObjects();

  this->setModified();
  this->render();
}

void pqFourFlowPlaneWidget::createWidget(pqServer* server, const QString& xmlname) {
	std::cout<<"pqFourFlowPlaneWidget::createWidget"<<std::endl;
  vtkSMNewWidgetRepresentationProxy* const widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->get3DWidget(
      xmlname, server);
  this->setWidgetProxy(widget);

  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();
}

void pqFourFlowPlaneWidget::resetBounds(double bounds[6]) {
	std::cout<<"pqFourFlowPlaneWidget::resetBounds"<<std::endl;
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if(vtkSMDoubleVectorProperty* const place_widget =
    vtkSMDoubleVectorProperty::SafeDownCast(
      widget->GetProperty("PlaceWidget")))
    {
    place_widget->SetElements(bounds);
    widget->UpdateProperty("PlaceWidget", 1);
    }
  widget->UpdatePropertyInformation();
}

void pqFourFlowPlaneWidget::onWidgetVisibilityChanged(bool visible) {
	std::cout<<"pqFourFlowPlaneWidget::onWidgetVisibilityChanged"<<std::endl;
}
