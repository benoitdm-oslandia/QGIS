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

  // height
  ++wdgIdx;
  mSpinHeight = new QDoubleSpinBox( mMainGroupBox );
  mSpinHeight->setObjectName( "mSpinHeight" );
  mSpinHeight->setMinimum( 0.0001 );
  mSpinHeight->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinHeight );

  QLabel *labelHeight = new QLabel( mMainGroupBox );
  labelHeight->setObjectName( "labelHeight" );
  labelHeight->setText( tr( "Height" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelHeight );

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
  connect( mSpinHeight, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinRadial, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveCylinderDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setRadius( 1.0 );
  setHeight( 1.0 );
  setRadial( 12 );
}

void Qgs3DCreatePrimitiveCylinderDialog::setRadius( double size )
{
  whileBlocking( mSpinRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveCylinderDialog::setHeight( double size )
{
  whileBlocking( mSpinHeight )->setValue( size );
}

void Qgs3DCreatePrimitiveCylinderDialog::setRadial( int size )
{
  whileBlocking( mSpinRadial )->setValue( size );
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

QAbstractSpinBox *Qgs3DCreatePrimitiveCylinderDialog::getSpinBox( int idx )
{
  if ( idx == 0 )
  {
    return mSpinRadius;
  }
  if ( idx == 1 )
  {
    return mSpinHeight;
  }
  if ( idx == 2 )
  {
    return mSpinRadial;
  }
  return nullptr;
}
