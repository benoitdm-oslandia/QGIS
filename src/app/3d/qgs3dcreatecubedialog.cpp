#include "qgs3dcreatecubedialog.h"


Qgs3DCreateCubeDialog::Qgs3DCreateCubeDialog( Qt::WindowFlags f )
  : QDialog( nullptr, f )
{
  setupUi( this );

  setWindowTitle( tr( "Create Cube" ) );
}

void Qgs3DCreateCubeDialog::setTranslation( const QgsPoint &point )
{
  whileBlocking( spinTX )->setValue( point.x() );
  whileBlocking( spinTY )->setValue( point.y() );
  whileBlocking( spinTZ )->setValue( point.z() );
}

void Qgs3DCreateCubeDialog::setRotation( double rx, double ry, double rz )
{
  whileBlocking( spinRX )->setValue( rx );
  whileBlocking( spinRY )->setValue( ry );
  whileBlocking( spinRZ )->setValue( rz );
}

void Qgs3DCreateCubeDialog::resetData()
{
  // whileBlocking( spinTX )->setValue( point.x() );
  // whileBlocking( spinTY )->setValue( point.y() );
  // whileBlocking( spinTZ )->setValue( point.z() );
}

void Qgs3DCreateCubeDialog::setSize( double size )
{
  whileBlocking( spinSize )->setValue( size );
}
