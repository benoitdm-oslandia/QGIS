#include "qgs3dcreateprimitivecubedialog.h"

Qgs3DCreatePrimitiveCubeDialog::Qgs3DCreatePrimitiveCubeDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "cube", f )
{
  mSpinSizeX = new QDoubleSpinBox( mMainGroupBox );
  mSpinSizeX->setObjectName( "mSpinSizeX" );
  mSpinSizeX->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 1, QFormLayout::FieldRole, mSpinSizeX );

  QLabel *labelParamX = new QLabel( mMainGroupBox );
  labelParamX->setObjectName( "labelParamX" );
  labelParamX->setText( tr( "Size X" ) );
  paramFormLayout->setWidget( 1, QFormLayout::LabelRole, labelParamX );

  connect( mSpinSizeX, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  mSpinSizeY = new QDoubleSpinBox( mMainGroupBox );
  mSpinSizeY->setObjectName( "mSpinSizeY" );
  mSpinSizeY->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 2, QFormLayout::FieldRole, mSpinSizeY );

  QLabel *labelParamY = new QLabel( mMainGroupBox );
  labelParamY->setObjectName( "labelParamY" );
  labelParamY->setText( tr( "Size Y" ) );
  paramFormLayout->setWidget( 2, QFormLayout::LabelRole, labelParamY );

  connect( mSpinSizeY, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  mSpinSizeZ = new QDoubleSpinBox( mMainGroupBox );
  mSpinSizeZ->setObjectName( "mSpinSizeZ" );
  mSpinSizeZ->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 3, QFormLayout::FieldRole, mSpinSizeZ );

  QLabel *labelParamZ = new QLabel( mMainGroupBox );
  labelParamZ->setObjectName( "labelParamZ" );
  labelParamZ->setText( tr( "Size Z" ) );
  paramFormLayout->setWidget( 3, QFormLayout::LabelRole, labelParamZ );

  connect( mSpinSizeZ, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveCubeDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setSizeX( 1.0 );
  setSizeY( 1.0 );
  setSizeZ( 1.0 );
}

void Qgs3DCreatePrimitiveCubeDialog::setSizeX( double size )
{
  whileBlocking( mSpinSizeX )->setValue( size );
}
void Qgs3DCreatePrimitiveCubeDialog::setSizeY( double size )
{
  whileBlocking( mSpinSizeY )->setValue( size );
}
void Qgs3DCreatePrimitiveCubeDialog::setSizeZ( double size )
{
  whileBlocking( mSpinSizeZ )->setValue( size );
}

void Qgs3DCreatePrimitiveCubeDialog::setParam( int idx, double value )
{
  if ( idx == 0 )
  {
    setSizeX( value );
  }
  else if ( idx == 1 )
  {
    setSizeY( value );
  }
  else if ( idx == 2 )
  {
    setSizeZ( value );
  }
}

double Qgs3DCreatePrimitiveCubeDialog::getParam( int idx ) const
{
  if ( idx == 0 )
  {
    return mSpinSizeX->value();
  }
  if ( idx == 1 )
  {
    return mSpinSizeY->value();
  }
  if ( idx == 2 )
  {
    return mSpinSizeZ->value();
  }

  return std::numeric_limits<double>::quiet_NaN();
}

Qgs3DCreatePrimitiveDialog::ConstrainedAxis Qgs3DCreatePrimitiveCubeDialog::constrainedAxisForParam( int idx )
{
  if ( idx == 0 )
  {
    return Qgs3DCreatePrimitiveDialog::X;
  }
  if ( idx == 1 )
  {
    return Qgs3DCreatePrimitiveDialog::Y;
  }
  if ( idx == 2 )
  {
    return Qgs3DCreatePrimitiveDialog::Z;
  }

  return Qgs3DCreatePrimitiveDialog::NONE;
}
