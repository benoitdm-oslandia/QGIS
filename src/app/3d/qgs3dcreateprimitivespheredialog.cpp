#include "qgs3dcreateprimitivespheredialog.h"


Qgs3DCreatePrimitiveSphereDialog::Qgs3DCreatePrimitiveSphereDialog( Qt::WindowFlags f )
  : Qgs3DCreatePrimitiveDialog( "sphere", f )
{
  // radius
  mSpinSize = new QDoubleSpinBox( mMainGroupBox );
  mSpinSize->setObjectName( "spinSize" );
  mSpinSize->setMaximum( 99999999.989999994635582 );
  paramFormLayout->setWidget( 1, QFormLayout::FieldRole, mSpinSize );

  QLabel *mLabelParam1 = new QLabel( mMainGroupBox );
  mLabelParam1->setObjectName( "labelParam1" );
  mLabelParam1->setText( tr( "Radius" ) );
  paramFormLayout->setWidget( 1, QFormLayout::LabelRole, mLabelParam1 );

  // segment 1
  mSpinSegment1 = new QSpinBox( mMainGroupBox );
  mSpinSegment1->setObjectName( "mSpinSegment1" );
  mSpinSegment1->setMaximum( 64 );
  paramFormLayout->setWidget( 2, QFormLayout::FieldRole, mSpinSegment1 );

  QLabel *mLabelSegment1 = new QLabel( mMainGroupBox );
  mLabelSegment1->setObjectName( "mLabelSegment1" );
  mLabelSegment1->setText( tr( "Segment 1" ) );
  paramFormLayout->setWidget( 2, QFormLayout::LabelRole, mLabelSegment1 );

  // segment 2
  mSpinSegment2 = new QSpinBox( mMainGroupBox );
  mSpinSegment2->setObjectName( "mSpinSegment2" );
  mSpinSegment2->setMaximum( 64 );
  paramFormLayout->setWidget( 3, QFormLayout::FieldRole, mSpinSegment2 );

  QLabel *mLabelSegment2 = new QLabel( mMainGroupBox );
  mLabelSegment2->setObjectName( "mLabelSegment2" );
  mLabelSegment2->setText( tr( "Segment 2" ) );
  paramFormLayout->setWidget( 3, QFormLayout::LabelRole, mLabelSegment2 );

  connect( mSpinSize, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinSegment1, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( mSpinSegment2, &QSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  resetData();
}

void Qgs3DCreatePrimitiveSphereDialog::resetData()
{
  Qgs3DCreatePrimitiveDialog::resetData();
  setRadius( 1.0 );
  setSegment1( 4 );
  setSegment2( 4 );
}

void Qgs3DCreatePrimitiveSphereDialog::setRadius( double size )
{
  whileBlocking( mSpinSize )->setValue( size );
}

void Qgs3DCreatePrimitiveSphereDialog::setSegment1( int size )
{
  whileBlocking( mSpinSegment1 )->setValue( size );
}

void Qgs3DCreatePrimitiveSphereDialog::setSegment2( int size )
{
  whileBlocking( mSpinSegment2 )->setValue( size );
}
