#include "qgs3dcreateprimitiveconedialog.h"

Qgs3DCreatePrimitiveConeDialog::Qgs3DCreatePrimitiveConeDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "cone", f )
{
  int wdgIdx = 0;

  // bottom radius
  ++wdgIdx;
  mSpinBottomRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinBottomRadius->setObjectName( "mSpinBottomRadius" );
  mSpinBottomRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinBottomRadius );

  QLabel *labelBottomRadius = new QLabel( mMainGroupBox );
  labelBottomRadius->setObjectName( "labelBottomRadius" );
  labelBottomRadius->setText( tr( "BottomRadius" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelBottomRadius );

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

  // top radius
  ++wdgIdx;
  mSpinTopRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinTopRadius->setObjectName( "mSpinTopRadius" );
  mSpinTopRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinTopRadius );

  QLabel *labelTopRadius = new QLabel( mMainGroupBox );
  labelTopRadius->setObjectName( "labelTopRadius" );
  labelTopRadius->setText( tr( "TopRadius" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelTopRadius );

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

  connect( mSpinBottomRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinLength, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinTopRadius, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinRings, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinSlices, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveConeDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setBottomRadius( 1.0 );
  setLength( 1.0 );
  setTopRadius( 1.0 );
  setRings( 4 );
  setSlices( 4 );
}

void Qgs3DCreatePrimitiveConeDialog::setBottomRadius( double size )
{
  whileBlocking( mSpinBottomRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveConeDialog::setTopRadius( double size )
{
  whileBlocking( mSpinTopRadius )->setValue( size );
}

void Qgs3DCreatePrimitiveConeDialog::setLength( double size )
{
  whileBlocking( mSpinLength )->setValue( size );
}

void Qgs3DCreatePrimitiveConeDialog::setRings( int size )
{
  whileBlocking( mSpinRings )->setValue( size );
}

void Qgs3DCreatePrimitiveConeDialog::setSlices( int size )
{
  whileBlocking( mSpinSlices )->setValue( size );
}

void Qgs3DCreatePrimitiveConeDialog::setParam( int idx, double value )
{
  if ( idx == 0 )
  {
    setBottomRadius( value );
  }
  else if ( idx == 1 )
  {
    setLength( value );
  }
  else if ( idx == 2 )
  {
    setTopRadius( value );
  }
}

double Qgs3DCreatePrimitiveConeDialog::getParam( int idx ) const
{
  if ( idx == 0 )
  {
    return mSpinBottomRadius->value();
  }
  if ( idx == 1 )
  {
    return mSpinLength->value();
  }
  if ( idx == 2 )
  {
    return mSpinTopRadius->value();
  }

  return std::numeric_limits<double>::quiet_NaN();
}
