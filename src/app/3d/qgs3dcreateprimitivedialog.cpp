#include "qgs3dcreateprimitivedialog.h"

Qgs3DCreatePrimitiveDialog::Qgs3DCreatePrimitiveDialog( const QString &type, Qt::WindowFlags f )
  : QDialog( nullptr, f ), mType( type )
{
  setupUi( this );

  setWindowTitle( QString( "Create " ) + mType );
  /*[this]( double ) { emit valueChanged(); }*/
  connect( spinTX, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( spinTY, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( spinTZ, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  connect( spinRX, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( spinRY, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( spinRZ, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  connect( spinSX, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( spinSY, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );
  connect( spinSZ, &QgsDoubleSpinBox::valueChanged, this, &Qgs3DCreatePrimitiveDialog::valueChanged );

  connect( createButton, &QPushButton::clicked, this, &Qgs3DCreatePrimitiveDialog::createClicked );
}

void Qgs3DCreatePrimitiveDialog::setTranslation( const QgsPoint &point )
{
  whileBlocking( spinTX )->setValue( point.x() );
  whileBlocking( spinTY )->setValue( point.y() );
  whileBlocking( spinTZ )->setValue( point.z() );
}

void Qgs3DCreatePrimitiveDialog::setScale( const QgsPoint &point )
{
  whileBlocking( spinSX )->setValue( point.x() );
  whileBlocking( spinSY )->setValue( point.y() );
  whileBlocking( spinSZ )->setValue( point.z() );
}

void Qgs3DCreatePrimitiveDialog::setRotation( double rx, double ry, double rz )
{
  whileBlocking( spinRX )->setValue( rx );
  whileBlocking( spinRY )->setValue( ry );
  whileBlocking( spinRZ )->setValue( rz );
}

void Qgs3DCreatePrimitiveDialog::resetData()
{
  setRotation( 0.0, 0.0, 0.0 );
  setTranslation( { 0.0, 0.0, 0.0 } );
}

void Qgs3DCreatePrimitiveDialog::unfocusCreateButton()
{
  createButton->clearFocus();
  transformationWidget->setFocus( Qt::TabFocusReason );
}

void Qgs3DCreatePrimitiveDialog::focusCreateButton()
{
  createButton->setFocus( Qt::TabFocusReason );
}


void Qgs3DCreatePrimitiveDialog::setParam( int idx, double value )
{
  QAbstractSpinBox *spin = getSpinBox( idx );
  if ( spin != nullptr )
  {
    if ( auto qSpin = dynamic_cast<QSpinBox *>( spin ) )
    {
      whileBlocking( qSpin )->setValue( value );
    }
    else if ( auto qSpin = dynamic_cast<QDoubleSpinBox *>( spin ) )
    {
      whileBlocking( qSpin )->setValue( value );
    }
  }
}

double Qgs3DCreatePrimitiveDialog::getParam( int idx ) const
{
  const QAbstractSpinBox *spin = const_cast<Qgs3DCreatePrimitiveDialog *>( this )->getSpinBox( idx );
  if ( spin != nullptr )
  {
    if ( auto qSpin = dynamic_cast<const QSpinBox *>( spin ) )
    {
      return qSpin->value();
    }
    if ( auto qSpin = dynamic_cast<const QDoubleSpinBox *>( spin ) )
    {
      return qSpin->value();
    }
  }

  return std::numeric_limits<double>::quiet_NaN();
}

void Qgs3DCreatePrimitiveDialog::focusOnParam( int idx )
{
  for ( int i = 0; i < paramNumber(); ++i )
  {
    QAbstractSpinBox *spin = getSpinBox( i );
    if ( spin != nullptr )
    {
      if ( i == idx )
        spin->setFocus( Qt::TabFocusReason );
      else
        spin->clearFocus();
    }
  }
}
