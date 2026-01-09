#include "qgs3dcreateprimitivespheredialog.h"

Qgs3DCreatePrimitiveSphereDialog::Qgs3DCreatePrimitiveSphereDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "sphere", f )
{
  // radius
  mSpinRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinRadius->setObjectName( "mSpinRadius" );
  mSpinRadius->setMinimum( 0.0001 );
  mSpinRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 1, QFormLayout::FieldRole, mSpinRadius );

  QLabel *labelRadius = new QLabel( mMainGroupBox );
  labelRadius->setObjectName( "labelRadius" );
  labelRadius->setText( tr( "Radius" ) );
  paramFormLayout->setWidget( 1, QFormLayout::LabelRole, labelRadius );

  // segment 1
  mSpinRings = new QSpinBox( mMainGroupBox );
  mSpinRings->setObjectName( "mSpinRings" );
  mSpinRings->setMinimum( 3 );
  mSpinRings->setMaximum( 64 );
  paramFormLayout->setWidget( 2, QFormLayout::FieldRole, mSpinRings );

  QLabel *labelRings = new QLabel( mMainGroupBox );
  labelRings->setObjectName( "labelRings" );
  labelRings->setText( tr( "Ring" ) );
  paramFormLayout->setWidget( 2, QFormLayout::LabelRole, labelRings );

  // segment 2
  mSpinSlices = new QSpinBox( mMainGroupBox );
  mSpinSlices->setObjectName( "mSpinSlices" );
  mSpinSlices->setMinimum( 3 );
  mSpinSlices->setMaximum( 64 );
  paramFormLayout->setWidget( 3, QFormLayout::FieldRole, mSpinSlices );

  QLabel *labelSlices = new QLabel( mMainGroupBox );
  labelSlices->setObjectName( "labelSlices" );
  labelSlices->setText( tr( "Slice" ) );
  paramFormLayout->setWidget( 3, QFormLayout::LabelRole, labelSlices );

  connect( mSpinRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinRings, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinSlices, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveSphereDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setRadius( 1.0 );
  setRings( 4 );
  setSlices( 4 );
}

void Qgs3DCreatePrimitiveSphereDialog::setRadius( double size )
{
  whileBlocking( mSpinRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveSphereDialog::setRings( int size )
{
  whileBlocking( mSpinRings )->setValue( size );
}

void Qgs3DCreatePrimitiveSphereDialog::setSlices( int size )
{
  whileBlocking( mSpinSlices )->setValue( size );
}

Qgs3DCreatePrimitiveDialog::ConstrainedAxis Qgs3DCreatePrimitiveSphereDialog::constrainedAxisForParam( int idx )
{
  if ( idx == 0 )
  {
    return Qgs3DCreatePrimitiveDialog::XY;
  }

  return Qgs3DCreatePrimitiveDialog::NONE;
}

QAbstractSpinBox *Qgs3DCreatePrimitiveSphereDialog::getSpinBox( int idx )
{
  if ( idx == 0 )
  {
    return mSpinRadius;
  }
  if ( idx == 1 )
  {
    return mSpinRings;
  }
  if ( idx == 2 )
  {
    return mSpinSlices;
  }

  return nullptr;
}
