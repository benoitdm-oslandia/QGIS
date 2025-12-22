#include "qgs3dcreateprimitivecylinderdialog.h"

Qgs3DCreatePrimitiveCylinderDialog::Qgs3DCreatePrimitiveCylinderDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "cylinder", f )
{
  int wdgIdx = 0;

  // radius
  ++wdgIdx;
  mSpinRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinRadius->setObjectName( "mSpinRadius" );
  mSpinRadius->setMinimum( 0.0001 );
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
  mSpinLength->setMinimum( 0.0001 );
  mSpinLength->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinLength );

  QLabel *labelLength = new QLabel( mMainGroupBox );
  labelLength->setObjectName( "labelLength" );
  labelLength->setText( tr( "Length" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelLength );

  // segment 1
  ++wdgIdx;
  mSpinRadial = new QSpinBox( mMainGroupBox );
  mSpinRadial->setObjectName( "mSpinRadial" );
  mSpinRadial->setMinimum( 3 );
  mSpinRadial->setMaximum( 64 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinRadial );

  QLabel *labelRadial = new QLabel( mMainGroupBox );
  labelRadial->setObjectName( "labelRadial" );
  labelRadial->setText( tr( "Radial" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelRadial );

  connect( mSpinRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinLength, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinRadial, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveCylinderDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setRadius( 1.0 );
  setLength( 1.0 );
  setRadial( 4 );
}

void Qgs3DCreatePrimitiveCylinderDialog::setRadius( double size )
{
  whileBlocking( mSpinRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveCylinderDialog::setLength( double size )
{
  whileBlocking( mSpinLength )->setValue( size );
}

void Qgs3DCreatePrimitiveCylinderDialog::setRadial( int size )
{
  whileBlocking( mSpinRadial )->setValue( size );
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
  else if ( idx == 2 )
  {
    setRadial( value );
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
  if ( idx == 2 )
  {
    return mSpinRadial->value();
  }
  return std::numeric_limits<double>::quiet_NaN();
}

Qgs3DCreatePrimitiveDialog::ConstrainedAxis Qgs3DCreatePrimitiveCylinderDialog::constrainedAxisForParam( int idx )
{
  if ( idx == 0 )
  {
    return Qgs3DCreatePrimitiveDialog::XY;
  }
  if ( idx == 1 )
  {
    return Qgs3DCreatePrimitiveDialog::Z;
  }

  return Qgs3DCreatePrimitiveDialog::NONE;
}
