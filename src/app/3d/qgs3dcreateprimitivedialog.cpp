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
  transformationWidget->setFocus( Qt::OtherFocusReason );
}

void Qgs3DCreatePrimitiveDialog::focusCreateButton()
{
  createButton->setFocus( Qt::OtherFocusReason );
}
