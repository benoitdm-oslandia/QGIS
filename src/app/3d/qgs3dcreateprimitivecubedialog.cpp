#include "qgs3dcreateprimitivecubedialog.h"


Qgs3DCreatePrimitiveCubeDialog::Qgs3DCreatePrimitiveCubeDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "cube", f )
{
  mSpinSize = new QDoubleSpinBox( mMainGroupBox );
  mSpinSize->setObjectName( "spinSize" );
  mSpinSize->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 1, QFormLayout::FieldRole, mSpinSize );

  QLabel *mLabelParam1 = new QLabel( mMainGroupBox );
  mLabelParam1->setObjectName( "labelParam1" );
  mLabelParam1->setText( tr( "Size" ) );
  paramFormLayout->setWidget( 1, QFormLayout::LabelRole, mLabelParam1 );

  connect( mSpinSize, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveCubeDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setSize( 1.0 );
}

void Qgs3DCreatePrimitiveCubeDialog::setSize( double size )
{
  whileBlocking( mSpinSize )->setValue( size );
}
