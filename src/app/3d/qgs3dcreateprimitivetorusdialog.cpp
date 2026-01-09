#include "qgs3dcreateprimitivetorusdialog.h"

Qgs3DCreatePrimitiveTorusDialog::Qgs3DCreatePrimitiveTorusDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "torus", f )
{
  // radius
  mSpinMainRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinMainRadius->setObjectName( "mSpinMainRadius" );
  mSpinMainRadius->setMinimum( 0.0001 );
  mSpinMainRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 1, QFormLayout::FieldRole, mSpinMainRadius );

  QLabel *labelMainRadius = new QLabel( mMainGroupBox );
  labelMainRadius->setObjectName( "labelMainRadius" );
  labelMainRadius->setText( tr( "Main radius" ) );
  paramFormLayout->setWidget( 1, QFormLayout::LabelRole, labelMainRadius );

  // minorradius
  mSpinTubeRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinTubeRadius->setObjectName( "mSpinMinorRadius" );
  mSpinTubeRadius->setMinimum( 0.0001 );
  mSpinTubeRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 2, QFormLayout::FieldRole, mSpinTubeRadius );

  QLabel *labelTubeRadius = new QLabel( mMainGroupBox );
  labelTubeRadius->setObjectName( "labelTubeRadius" );
  labelTubeRadius->setText( tr( "Tube radius" ) );
  paramFormLayout->setWidget( 2, QFormLayout::LabelRole, labelTubeRadius );

  // segment 1
  mSpinMainRadial = new QSpinBox( mMainGroupBox );
  mSpinMainRadial->setObjectName( "mSpinMainRadial" );
  mSpinMainRadial->setMinimum( 3 );
  mSpinMainRadial->setMaximum( 64 );
  paramFormLayout->setWidget( 3, QFormLayout::FieldRole, mSpinMainRadial );

  QLabel *labelMainRadial = new QLabel( mMainGroupBox );
  labelMainRadial->setObjectName( "labelMainRadial" );
  labelMainRadial->setText( tr( "Main radial" ) );
  paramFormLayout->setWidget( 3, QFormLayout::LabelRole, labelMainRadial );

  // segment 2
  mSpinTubeRadial = new QSpinBox( mMainGroupBox );
  mSpinTubeRadial->setObjectName( "mSpinTubeRadial" );
  mSpinTubeRadial->setMinimum( 3 );
  mSpinTubeRadial->setMaximum( 64 );
  paramFormLayout->setWidget( 4, QFormLayout::FieldRole, mSpinTubeRadial );

  QLabel *labelTubeRadial = new QLabel( mMainGroupBox );
  labelTubeRadial->setObjectName( "labelTubeRadial" );
  labelTubeRadial->setText( tr( "Tube radial" ) );
  paramFormLayout->setWidget( 4, QFormLayout::LabelRole, labelTubeRadial );

  connect( mSpinMainRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinTubeRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinMainRadial, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinTubeRadial, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveTorusDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setMainRadius( 1.0 );
  setTubeRadius( 1.0 );
  setMainRadial( 4 );
  setTubeRadial( 4 );
}

void Qgs3DCreatePrimitiveTorusDialog::setMainRadius( double size )
{
  whileBlocking( mSpinMainRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveTorusDialog::setTubeRadius( double size )
{
  whileBlocking( mSpinTubeRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveTorusDialog::setMainRadial( int size )
{
  whileBlocking( mSpinMainRadial )->setValue( size );
}

void Qgs3DCreatePrimitiveTorusDialog::setTubeRadial( int size )
{
  whileBlocking( mSpinTubeRadial )->setValue( size );
}

Qgs3DCreatePrimitiveDialog::ConstrainedAxis Qgs3DCreatePrimitiveTorusDialog::constrainedAxisForParam( int idx )
{
  if ( idx == 0 )
  {
    return Qgs3DCreatePrimitiveDialog::X;
  }
  if ( idx == 1 )
  {
    return Qgs3DCreatePrimitiveDialog::Y;
  }

  return Qgs3DCreatePrimitiveDialog::NONE;
}

QAbstractSpinBox *Qgs3DCreatePrimitiveTorusDialog::getSpinBox( int idx )
{
  if ( idx == 0 )
  {
    return mSpinMainRadius;
  }
  if ( idx == 1 )
  {
    return mSpinTubeRadius;
  }
  if ( idx == 2 )
  {
    return mSpinMainRadial;
  }
  if ( idx == 3 )
  {
    return mSpinTubeRadial;
  }
  return nullptr;
}
