#include "qgs3dcreateprimitivecylinderdialog.h"

Qgs3DCreatePrimitiveCylinderDialog::Qgs3DCreatePrimitiveCylinderDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "cylinder", f )
{
  int wdgIdx = 0;

  // radius
  ++wdgIdx;
  mSpinRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinRadius->setObjectName( "mSpinRadius" );
  mSpinRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinRadius );

  QLabel *labelRadius = new QLabel( mMainGroupBox );
  labelRadius->setObjectName( "labelRadius" );
  labelRadius->setText( tr( "Radius" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelRadius );

  // length
  ++wdgIdx;
  mSpinLength = new QDoubleSpinBox( mMainGroupBox );
  mSpinLength->setObjectName( "mSpinLength" );
  mSpinLength->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinLength );

  QLabel *labelLength = new QLabel( mMainGroupBox );
  labelLength->setObjectName( "labelLength" );
  labelLength->setText( tr( "Length" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelLength );

  // segment 1
  ++wdgIdx;
  mSpinRings = new QSpinBox( mMainGroupBox );
  mSpinRings->setObjectName( "mSpinRings" );
  mSpinRings->setMaximum( 64 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinRings );

  QLabel *labelRings = new QLabel( mMainGroupBox );
  labelRings->setObjectName( "labelRings" );
  labelRings->setText( tr( "Segment 1" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelRings );

  // segment 2
  ++wdgIdx;
  mSpinSlices = new QSpinBox( mMainGroupBox );
  mSpinSlices->setObjectName( "mSpinSlices" );
  mSpinSlices->setMaximum( 64 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinSlices );

  QLabel *labelSlices = new QLabel( mMainGroupBox );
  labelSlices->setObjectName( "labelSlices" );
  labelSlices->setText( tr( "Segment 2" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelSlices );

  connect( mSpinRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinLength, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinRings, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinSlices, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveCylinderDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setRadius( 1.0 );
  setLength( 1.0 );
  setRings( 4 );
  setSlices( 4 );
}

void Qgs3DCreatePrimitiveCylinderDialog::setRadius( double size )
{
  whileBlocking( mSpinRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveCylinderDialog::setLength( double size )
{
  whileBlocking( mSpinLength )->setValue( size );
}

void Qgs3DCreatePrimitiveCylinderDialog::setRings( int size )
{
  whileBlocking( mSpinRings )->setValue( size );
}

void Qgs3DCreatePrimitiveCylinderDialog::setSlices( int size )
{
  whileBlocking( mSpinSlices )->setValue( size );
}

void Qgs3DCreatePrimitiveCylinderDialog::setParam( int idx, double value )
{
  if ( idx == 0 )
  {
    setRadius( value );
  }
  else if ( idx == 1 )
  {
    setLength( value );
  }
}

double Qgs3DCreatePrimitiveCylinderDialog::getParam( int idx ) const
{
  if ( idx == 0 )
  {
    return mSpinRadius->value();
  }
  if ( idx == 1 )
  {
    return mSpinLength->value();
  }

  return std::numeric_limits<double>::quiet_NaN();
}
