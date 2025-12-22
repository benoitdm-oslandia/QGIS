#include "qgs3dcreateprimitiveconedialog.h"

Qgs3DCreatePrimitiveConeDialog::Qgs3DCreatePrimitiveConeDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "cone", f )
{
  int wdgIdx = 0;

  // bottom radius
  ++wdgIdx;
  mSpinBottomRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinBottomRadius->setObjectName( "mSpinBottomRadius" );
  mSpinBottomRadius->setMinimum( 0.0001 );
  mSpinBottomRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinBottomRadius );

  QLabel *labelBottomRadius = new QLabel( mMainGroupBox );
  labelBottomRadius->setObjectName( "labelBottomRadius" );
  labelBottomRadius->setText( tr( "Bottom radius" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelBottomRadius );

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

  // top radius
  ++wdgIdx;
  mSpinTopRadius = new QDoubleSpinBox( mMainGroupBox );
  mSpinTopRadius->setObjectName( "mSpinTopRadius" );
  mSpinTopRadius->setMinimum( 0.0001 );
  mSpinTopRadius->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::FieldRole, mSpinTopRadius );

  QLabel *labelTopRadius = new QLabel( mMainGroupBox );
  labelTopRadius->setObjectName( "labelTopRadius" );
  labelTopRadius->setText( tr( "Top radius" ) );
  paramFormLayout->setWidget( wdgIdx, QFormLayout::LabelRole, labelTopRadius );

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

  resetData();
}

void Qgs3DCreatePrimitiveConeDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setBottomRadius( 1.0 );
  setLength( 1.0 );
  setTopRadius( 1.0 );
  setRadial( 4 );
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

void Qgs3DCreatePrimitiveConeDialog::setRadial( int size )
{
  whileBlocking( mSpinRadial )->setValue( size );
}

Qgs3DCreatePrimitiveDialog::ConstrainedAxis Qgs3DCreatePrimitiveConeDialog::constrainedAxisForParam( int idx )
{
  if ( idx == 0 )
  {
    return Qgs3DCreatePrimitiveDialog::XY;
  }
  if ( idx == 1 )
  {
    return Qgs3DCreatePrimitiveDialog::Z;
  }
  if ( idx == 2 )
  {
    return Qgs3DCreatePrimitiveDialog::XY;
  }

  return Qgs3DCreatePrimitiveDialog::NONE;
}

QAbstractSpinBox *Qgs3DCreatePrimitiveConeDialog::getSpinBox( int idx )
{
  if ( idx == 0 )
  {
    return mSpinBottomRadius;
  }
  if ( idx == 1 )
  {
    return mSpinLength;
  }
  if ( idx == 2 )
  {
    return mSpinTopRadius;
  }
  if ( idx == 3 )
  {
    return mSpinRadial;
  }

  return nullptr;
}
