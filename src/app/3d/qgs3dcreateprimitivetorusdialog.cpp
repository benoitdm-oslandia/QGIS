#include "qgs3dcreateprimitivetorusdialog.h"

Qgs3DCreatePrimitiveTorusDialog::Qgs3DCreatePrimitiveTorusDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "torus", f )
{
  // radius
  mSpinRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinRadius->setObjectName( "mSpinRadius" );
  mSpinRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 1, QFormLayout::FieldRole, mSpinRadius );

  QLabel *labelRadius = new QLabel( mMainGroupBox );
  labelRadius->setObjectName( "labelRadius" );
  labelRadius->setText( tr( "Radius" ) );
  paramFormLayout->setWidget( 1, QFormLayout::LabelRole, labelRadius );

  // minorradius
  mSpinMinorRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinMinorRadius->setObjectName( "mSpinMinorRadius" );
  mSpinMinorRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 2, QFormLayout::FieldRole, mSpinMinorRadius );

  QLabel *labelMinorRadius = new QLabel( mMainGroupBox );
  labelMinorRadius->setObjectName( "labelMinorRadius" );
  labelMinorRadius->setText( tr( "Minor radius" ) );
  paramFormLayout->setWidget( 2, QFormLayout::LabelRole, labelMinorRadius );

  // segment 1
  mSpinRings = new QSpinBox( mMainGroupBox );
  mSpinRings->setObjectName( "mSpinRings" );
  mSpinRings->setMaximum( 64 );
  paramFormLayout->setWidget( 3, QFormLayout::FieldRole, mSpinRings );

  QLabel *labelRings = new QLabel( mMainGroupBox );
  labelRings->setObjectName( "labelRings" );
  labelRings->setText( tr( "Segment 1" ) );
  paramFormLayout->setWidget( 3, QFormLayout::LabelRole, labelRings );

  // segment 2
  mSpinSlices = new QSpinBox( mMainGroupBox );
  mSpinSlices->setObjectName( "mSpinSlices" );
  mSpinSlices->setMaximum( 64 );
  paramFormLayout->setWidget( 4, QFormLayout::FieldRole, mSpinSlices );

  QLabel *labelSlices = new QLabel( mMainGroupBox );
  labelSlices->setObjectName( "labelSlices" );
  labelSlices->setText( tr( "Segment 2" ) );
  paramFormLayout->setWidget( 4, QFormLayout::LabelRole, labelSlices );

  connect( mSpinRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinMinorRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinRings, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinSlices, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );


  resetData();
}

void Qgs3DCreatePrimitiveTorusDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setRadius( 1.0 );
  setMinorRadius( 1.0 );
  setRings( 4 );
  setSlices( 4 );
}

void Qgs3DCreatePrimitiveTorusDialog::setRadius( double size )
{
  whileBlocking( mSpinRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveTorusDialog::setMinorRadius( double size )
{
  whileBlocking( mSpinMinorRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveTorusDialog::setRings( int size )
{
  whileBlocking( mSpinRings )->setValue( size );
}

void Qgs3DCreatePrimitiveTorusDialog::setSlices( int size )
{
  whileBlocking( mSpinSlices )->setValue( size );
}

void Qgs3DCreatePrimitiveTorusDialog::setParam( int idx, double value )
{
  if ( idx == 0 )
  {
    setRadius( value );
  }
  else if ( idx == 1 )
  {
    setMinorRadius( value );
  }
}

double Qgs3DCreatePrimitiveTorusDialog::getParam( int idx ) const
{
  if ( idx == 0 )
  {
    return mSpinRadius->value();
  }
  if ( idx == 1 )
  {
    return mSpinMinorRadius->value();
  }
  return std::numeric_limits<double>::quiet_NaN();
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
