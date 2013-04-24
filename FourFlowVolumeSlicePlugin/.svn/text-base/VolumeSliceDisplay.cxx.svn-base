
#include "VolumeSliceDisplay.h"

#include <QVBoxLayout>
#include <QLabel>

VolumeSliceDisplay::VolumeSliceDisplay(pqRepresentation* d, QWidget* p)
  : pqDisplayPanel(d,p)
{
  // just make a label that shows we made it in the GUI
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(new QLabel("From Plugin", this));
}

VolumeSliceDisplay::~VolumeSliceDisplay()
{
}

