#include "qgssfcgalengine.h"
#include "qgsgeometry.h"
#include "qgspolygon.h"
#include "qgsgeometryfactory.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <charconv>

Q_GLOBAL_STATIC( sfcgal::Handler, sfcgalHandler )


void sfcgal::Deleter::operator()( sfcgal_geometry_t *geom ) const
{
  sfcgal_geometry_delete( geom );
}

void sfcgal::Deleter::operator()( const sfcgal_prepared_geometry_t *geom ) const
{
  sfcgal_prepared_geometry_delete( ( sfcgal_prepared_geometry_t * )geom );
}

bool sfcgal::Handler::hasFailed( QString *errorMsg )
{
  if ( !getErrorText().isEmpty() )
  {
    QgsDebugError( QString( "sfcgal operation failed: " ) + getErrorText() );
    if ( errorMsg )
    {
      errorMsg->clear();
      errorMsg->append( getErrorText() );
    }
    return true;
  }
  return false;
}

int sfcgal::errorHandler( const char *fmt, ... )
{
  va_list ap;
  char buffer[1024];

  va_start( ap, fmt );
  vsnprintf( buffer, sizeof buffer, fmt, ap );
  va_end( ap );

  sfcgalHandler->addErrorText( QStringLiteral( "SFCgal error occurred: %1" ).arg( buffer ) );

  return strlen( buffer );
}

int sfcgal::warningHandler( const char *fmt, ... )
{
  va_list ap;
  char buffer[1024];

  va_start( ap, fmt );
  vsnprintf( buffer, sizeof buffer, fmt, ap );
  va_end( ap );

  sfcgalHandler->addErrorText( QStringLiteral( "SFCgal warning occurred: %1" ).arg( buffer ) );

  return strlen( buffer );
}


sfcgal::Handler::Handler()
{
  sfcgal_init(); // empty but called
  sfcgal_set_error_handlers( sfcgal::warningHandler, sfcgal::errorHandler );
}

void sfcgal::Handler::clearErrorText()
{
  errorMessage.clear();
}

QString sfcgal::Handler::getErrorText() const
{
  return errorMessage;
}

void sfcgal::Handler::addErrorText( const QString &msg )
{
  qDebug() << "sfcgal::Handler::addErrorText" << msg;
  errorMessage.append( msg );
}

// Release return with delete []
unsigned char *sfcgal::hex2bytes( const char *hex, int *size )
{
  QByteArray ba = QByteArray::fromHex( hex );
  unsigned char *out = new unsigned char[ba.size()];
  memcpy( out, ba.data(), ba.size() );
  *size = ba.size();
  return out;
}

QString sfcgal::bytes2hex( const char *bytes, int size )
{
  QByteArray ba( bytes, size );
  QString out = ba.toHex();
  return out;
}

QgsSfcgalEngine::QgsSfcgalEngine( const QgsAbstractGeometry *geometry, double precision )
  : QgsGeometryEngine( geometry )
  , mPrecision( precision )
{
  mSfcgalGeom = fromAbsGeometry( geometry );
  QString errorMsg;
  if ( sfcgalHandler->hasFailed( &errorMsg ) )
    logError( QStringLiteral( "Unable to init sfcgal with geom." ), errorMsg );
}


std::unique_ptr<QgsAbstractGeometry> QgsSfcgalEngine::toAbsGeometry( const sfcgal::geometry *geom )
{
  sfcgalHandler->clearErrorText();
  std::unique_ptr<QgsAbstractGeometry> out;
  char *wkbHex;
  size_t len = 0;
  sfcgal_geometry_as_wkb( geom, &wkbHex, &len );
  if ( sfcgalHandler->hasFailed() )
    return nullptr;

  wkbHex [len] = 0; // temp fix bad ended array

  int len2 = 0;
  unsigned char *wkbBytesData = sfcgal::hex2bytes( wkbHex, &len2 );

  QgsConstWkbPtr ptr( ( const unsigned char * )wkbBytesData, len2 );
  out = QgsGeometryFactory::geomFromWkb( ptr );

  delete [] wkbBytesData;
  return out;
}

std::unique_ptr<QgsPolygon> QgsSfcgalEngine::toPolygon( const sfcgal::geometry *geom )
{
  std::unique_ptr<QgsPolygon> out;

  sfcgal_geometry_type_t type = sfcgal_geometry_type_id( geom );
  if ( type == SFCGAL_TYPE_POLYGON || type == SFCGAL_TYPE_MULTIPOLYGON )
  {
    out.reset( qgsgeometry_cast<QgsPolygon *>( toAbsGeometry( geom ).release() ) );
  }

  return out;
}

sfcgal::unique_ptr QgsSfcgalEngine::fromAbsGeometry( const QgsAbstractGeometry *geom, QString *errorMsg )
{
  sfcgalHandler->clearErrorText();
  QByteArray wkbBytes = geom->asWkb();
  QString wkbHex = sfcgal::bytes2hex( wkbBytes.data(), wkbBytes.size() );

  sfcgal::geometry *out = sfcgal_io_read_wkb( wkbHex.toStdString().c_str(), wkbHex.length() );

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  /*  sfcgal_geometry_is_valid(out);
    if ( sfcgalHandler->hasFailed( errorMsg ) )
      return nullptr;
  */

  return sfcgal::unique_ptr( out );
}

void QgsSfcgalEngine::geometryChanged()
{
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

void QgsSfcgalEngine::prepareGeometry()
{
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

QgsAbstractGeometry *QgsSfcgalEngine::intersection( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgalHandler->clearErrorText();
  sfcgal::unique_ptr other = fromAbsGeometry( geom, errorMsg );
  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal_geometry_t *result = nullptr;
  if ( geom->is3D() || mGeometry->is3D() )
    result = sfcgal_geometry_intersection_3d( mSfcgalGeom.get(), other.get() );
  else
    result = sfcgal_geometry_intersection( mSfcgalGeom.get(), other.get() );

  if ( !result )
    sfcgalHandler->addErrorText( "sfcgal produced null result." );

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  std::unique_ptr<QgsAbstractGeometry> out = toAbsGeometry( result );
  return out.release();
}

QgsAbstractGeometry *QgsSfcgalEngine::difference( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgalHandler->clearErrorText();
  sfcgal::unique_ptr other = fromAbsGeometry( geom, errorMsg );
  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal_geometry_t *result = nullptr;
  if ( geom->is3D() || mGeometry->is3D() )
    result = sfcgal_geometry_difference_3d( mSfcgalGeom.get(), other.get() );
  else
    result = sfcgal_geometry_difference( mSfcgalGeom.get(), other.get() ) ;

  if ( !result )
    sfcgalHandler->addErrorText( "sfcgal produced null result." );

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  std::unique_ptr<QgsAbstractGeometry> out = toAbsGeometry( result );
  return out.release();
}

QgsAbstractGeometry *QgsSfcgalEngine::combine( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgalHandler->clearErrorText();
  sfcgal::unique_ptr other = fromAbsGeometry( geom, errorMsg );
  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal_geometry_t *result = nullptr;
  if ( geom->is3D() || mGeometry->is3D() )
    result = sfcgal_geometry_union_3d( mSfcgalGeom.get(), other.get() );
  else
    result = sfcgal_geometry_union( mSfcgalGeom.get(), other.get() );

  if ( !result )
    sfcgalHandler->addErrorText( "sfcgal produced null result." );

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  std::unique_ptr<QgsAbstractGeometry> out = toAbsGeometry( result );
  return out.release();
}

QgsAbstractGeometry *QgsSfcgalEngine::combine( const QVector<QgsAbstractGeometry *> &geomList, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgalHandler->clearErrorText();
  sfcgal_geometry_t *combined = nullptr;
  for ( auto otherGeom : geomList )
  {
    sfcgal::unique_ptr other = fromAbsGeometry( otherGeom, errorMsg );
    if ( sfcgalHandler->hasFailed( errorMsg ) )
      return nullptr;

    if ( ! combined )
      combined = mSfcgalGeom.get();

    if ( otherGeom->is3D() || mGeometry->is3D() )
      combined =  sfcgal_geometry_union_3d( combined, other.get() ) ;
    else
      combined = sfcgal_geometry_union( combined, other.get() ) ;

    if ( !combined )
      sfcgalHandler->addErrorText( "sfcgal produced null result." );

    if ( sfcgalHandler->hasFailed( errorMsg ) )
      return nullptr;
  }

  std::unique_ptr<QgsAbstractGeometry> out = toAbsGeometry( combined );
  return out.release();
}

QgsAbstractGeometry *QgsSfcgalEngine::combine( const QVector<QgsGeometry> &geomList, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgalHandler->clearErrorText();
  sfcgal_geometry_t *combined = nullptr;
  for ( QgsGeometry otherGeom : geomList )
  {
    sfcgal::unique_ptr other = fromAbsGeometry( otherGeom.get() );
    if ( sfcgalHandler->hasFailed( errorMsg ) )
      return nullptr;

    if ( ! combined )
      combined = mSfcgalGeom.get();

    if ( otherGeom.get()->is3D() || mGeometry->is3D() )
      combined =  sfcgal_geometry_union_3d( combined, other.get() ) ;
    else
      combined = sfcgal_geometry_union( combined, other.get() ) ;

    if ( !combined )
      sfcgalHandler->addErrorText( "sfcgal produced null result." );

    if ( sfcgalHandler->hasFailed( errorMsg ) )
      return nullptr;
  }

  std::unique_ptr<QgsAbstractGeometry> out = toAbsGeometry( combined );
  return out.release();
}

QgsAbstractGeometry *QgsSfcgalEngine::symDifference( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters &parameters ) const
{
  Q_UNUSED( parameters );

  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return nullptr;
}

QgsAbstractGeometry *QgsSfcgalEngine::buffer( double distance, int, QString *errorMsg ) const
{
  sfcgalHandler->clearErrorText();
  sfcgal_geometry_t *result = nullptr;
  result =  sfcgal_geometry_round( mSfcgalGeom.get(), distance ) ;

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  std::unique_ptr<QgsAbstractGeometry> out = toAbsGeometry( result );
  return out.release();
}

QgsAbstractGeometry *QgsSfcgalEngine::buffer( double distance, int, Qgis::EndCapStyle, Qgis::JoinStyle, double, QString *errorMsg ) const
{
  return buffer( distance, 0, errorMsg );
}

QgsAbstractGeometry *QgsSfcgalEngine::simplify( double tolerance, QString *errorMsg ) const
{
  Q_UNUSED( tolerance );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return nullptr;
}

QgsAbstractGeometry *QgsSfcgalEngine::interpolate( double distance, QString *errorMsg ) const
{
  Q_UNUSED( distance );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return nullptr;
}

QgsAbstractGeometry *QgsSfcgalEngine::envelope( QString *errorMsg ) const
{
  sfcgalHandler->clearErrorText();
  sfcgal_geometry_t *result = nullptr;
  if ( mGeometry->is3D() )
    result =  sfcgal_geometry_convexhull( mSfcgalGeom.get() ) ;
  else
    result = sfcgal_geometry_convexhull_3d( mSfcgalGeom.get() ) ;

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  std::unique_ptr<QgsAbstractGeometry> out = toAbsGeometry( result );
  return out.release();
}

QgsPoint *QgsSfcgalEngine::centroid( QString *errorMsg ) const
{
  Q_UNUSED( errorMsg );
  return nullptr;

}

QgsPoint *QgsSfcgalEngine::pointOnSurface( QString *errorMsg ) const
{
  Q_UNUSED( errorMsg );
  return nullptr;

}

QgsAbstractGeometry *QgsSfcgalEngine::convexHull( QString *errorMsg ) const
{
  return envelope( errorMsg );

}

double QgsSfcgalEngine::distance( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  double result = std::numeric_limits<double>::quiet_NaN();
  sfcgalHandler->clearErrorText();
  sfcgal::unique_ptr other = fromAbsGeometry( geom, errorMsg );
  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return result;

  if ( mGeometry->is3D() )
    result =  sfcgal_geometry_distance_3d( mSfcgalGeom.get(), other.get() ) ;
  else
    result = sfcgal_geometry_distance( mSfcgalGeom.get(), other.get() ) ;

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return std::numeric_limits<double>::quiet_NaN();

  return result;
}

bool QgsSfcgalEngine::distanceWithin( const QgsAbstractGeometry *geom, double maxdistance, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  Q_UNUSED( maxdistance );
  return false;
}

bool QgsSfcgalEngine::intersects( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  sfcgalHandler->clearErrorText();
  sfcgal::unique_ptr other = fromAbsGeometry( geom, errorMsg );
  bool result = false;
  if ( geom->is3D() || mGeometry->is3D() )
    result = sfcgal_geometry_intersects_3d( mSfcgalGeom.get(), other.get() );
  else
    result = sfcgal_geometry_intersects( mSfcgalGeom.get(), other.get() );

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return false;

  return result;
}

bool QgsSfcgalEngine::touches( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::crosses( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::within( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::overlaps( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::contains( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  return false;
}

bool QgsSfcgalEngine::disjoint( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

QString QgsSfcgalEngine::relate( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return "false";
}

bool QgsSfcgalEngine::relatePattern( const QgsAbstractGeometry *geom, const QString &pattern, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  Q_UNUSED( pattern );

  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

double QgsSfcgalEngine::area( QString *errorMsg ) const
{
  double result = std::numeric_limits<double>::quiet_NaN();
  sfcgalHandler->clearErrorText();

  if ( mGeometry->is3D() )
    result =  sfcgal_geometry_area_3d( mSfcgalGeom.get() ) ;
  else
    result = sfcgal_geometry_area( mSfcgalGeom.get() ) ;

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return std::numeric_limits<double>::quiet_NaN();

  return result;
}

double QgsSfcgalEngine::length( QString *errorMsg ) const
{
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return 0.0;
}

bool QgsSfcgalEngine::isValid( QString *errorMsg, bool, QgsGeometry *errorLoc ) const
{
  sfcgalHandler->clearErrorText();
  bool result = false;
  char *reason;
  sfcgal_geometry_t *location;
  result = sfcgal_geometry_is_valid_detail( mSfcgalGeom.get(), &reason, &location );

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return false;

  if ( strlen( reason ) )
    sfcgalHandler->addErrorText( QString( reason ) );

  if ( location && errorLoc )
  {
    std::unique_ptr<QgsAbstractGeometry> locationGeom = toAbsGeometry( location );
    errorLoc->addPart( locationGeom.release() );
  }

  return result;
}

bool QgsSfcgalEngine::isEqual( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );

  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::isEmpty( QString *errorMsg ) const
{
  sfcgalHandler->clearErrorText();
  bool result = false;

  result = sfcgal_geometry_is_empty( mSfcgalGeom.get() );

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return false;

  return result;
}

bool QgsSfcgalEngine::isSimple( QString *errorMsg ) const
{
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

QgsGeometryEngine::EngineOperationResult QgsSfcgalEngine::splitGeometry( const QgsLineString &splitLine, QVector<QgsGeometry> &newGeometries, bool topological, QgsPointSequence &topologyTestPoints, QString *errorMsg, bool skipIntersectionCheck ) const
{
  Q_UNUSED( splitLine );
  Q_UNUSED( newGeometries );
  Q_UNUSED( topological );
  Q_UNUSED( topologyTestPoints );
  Q_UNUSED( skipIntersectionCheck );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return MethodNotImplemented;
}

QgsAbstractGeometry *QgsSfcgalEngine::offsetCurve( double distance, int, Qgis::JoinStyle, double, QString *errorMsg ) const
{
  sfcgalHandler->clearErrorText();
  sfcgal_geometry_t *result = nullptr;
  result =  sfcgal_geometry_offset_polygon( mSfcgalGeom.get(), distance ) ;

  if ( sfcgalHandler->hasFailed( errorMsg ) )
    return nullptr;

  std::unique_ptr<QgsAbstractGeometry> out = toAbsGeometry( result );
  return out.release();
}
