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
  mSpinSubdivisions = new QSpinBox( mMainGroupBox );
  mSpinSubdivisions->setObjectName( "mSpinSubdivisions" );
  mSpinSubdivisions->setMinimum( 1 );
  mSpinSubdivisions->setMaximum( 6 );
  paramFormLayout->setWidget( 2, QFormLayout::FieldRole, mSpinSubdivisions );

  QLabel *labelSubdivisions = new QLabel( mMainGroupBox );
  labelSubdivisions->setObjectName( "labelSubdivisions" );
  labelSubdivisions->setText( tr( "Subdivisions" ) );
  paramFormLayout->setWidget( 2, QFormLayout::LabelRole, labelSubdivisions );

  connect( mSpinRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinSubdivisions, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveSphereDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setRadius( 1.0 );
  setSubdivisions( 2 );
}

void Qgs3DCreatePrimitiveSphereDialog::setRadius( double size )
{
  whileBlocking( mSpinRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveSphereDialog::setSubdivisions( unsigned int subdivisions )
{
  whileBlocking( mSpinSubdivisions )->setValue( static_cast<int>( subdivisions ) );
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
    return mSpinSubdivisions;
  }

  return nullptr;
}
